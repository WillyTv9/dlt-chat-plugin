#include "dltchat/enhanced_retriever.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace dltchat {

namespace {

QStringList tokenise(const QString &query)
{
    static const QRegularExpression splitter(QStringLiteral("[^\\p{L}\\p{N}_]+"));
    QStringList raw = query.toLower().split(splitter, Qt::SkipEmptyParts);
    QStringList kept;
    kept.reserve(raw.size());
    for (const QString &t : raw) {
        if (t.size() >= 3)
            kept << t;
    }
    return kept;
}

qint64 parseTimestampMs(const QString &ts)
{
    if (ts.isEmpty())
        return 0;
    // Accept "HH:MM:SS.fff", "SS.fff", or plain int ms — best effort, never throws.
    bool ok = false;
    if (ts.contains(QLatin1Char(':'))) {
        QStringList parts = ts.split(QLatin1Char(':'));
        if (parts.size() == 3) {
            qint64 h = parts[0].toLongLong();
            qint64 m = parts[1].toLongLong();
            double s = parts[2].toDouble(&ok);
            return (h * 3600 + m * 60) * 1000 + static_cast<qint64>(s * 1000.0);
        }
    }
    double sec = ts.toDouble(&ok);
    if (ok)
        return static_cast<qint64>(sec * 1000.0);
    return ts.toLongLong();
}

QString diversityKey(const DltAnalyzerInterface::LogEntry &e)
{
    // Avoid double-picking entries that are essentially the same event.
    return e.category + QLatin1Char('|') + e.apid + QLatin1Char('|') + e.ctid;
}

} // namespace

const EnhancedRetriever::Config &EnhancedRetriever::defaultConfig()
{
    static const Config kDefault;
    return kDefault;
}

EnhancedRetriever::Result EnhancedRetriever::extract(
    const QString &query,
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QHash<QString, QSet<int>> &invertedIndex,
    const QList<int> &selectedIndices,
    const Config &config) const
{
    Result out;
    if (entries.isEmpty()) {
        out.diagnostic = QStringLiteral("entries=0 query=\"%1\"").arg(query);
        return out;
    }

    const QStringList tokens = tokenise(query);
    const int N = entries.size();

    QHash<int, int> indexToPos;
    indexToPos.reserve(N);
    for (int i = 0; i < N; ++i)
        indexToPos.insert(entries[i].index, i);

    // 1. Seed set: union of inverted-index hits for query tokens, capped.
    QSet<int> seedPositions;
    QHash<QString, double> idf;
    for (const QString &tok : tokens) {
        auto it = invertedIndex.find(tok);
        if (it == invertedIndex.end())
            continue;
        const int df = it.value().size();
        idf.insert(tok, std::log(static_cast<double>(N + 1) / static_cast<double>(df + 1)) + 1.0);
        for (int idx : it.value()) {
            auto posIt = indexToPos.find(idx);
            if (posIt != indexToPos.end())
                seedPositions.insert(posIt.value());
            if (seedPositions.size() >= config.maxSeedHits)
                break;
        }
        if (seedPositions.size() >= config.maxSeedHits)
            break;
    }

    // Fold in user-selected indices as forced seeds.
    for (int sel : selectedIndices) {
        auto posIt = indexToPos.find(sel);
        if (posIt != indexToPos.end())
            seedPositions.insert(posIt.value());
    }

    // 2. Score seeds. Recency uses position order (entries are append-ordered);
    //    correlation boost = exists another seed within correlationWindowMs.
    struct Scored
    {
        int pos;
        double score;
    };
    QVector<Scored> scored;
    scored.reserve(seedPositions.size());

    QVector<qint64> seedTimes;
    seedTimes.reserve(seedPositions.size());
    for (int pos : seedPositions)
        seedTimes.append(parseTimestampMs(entries[pos].timestamp));

    for (int pos : seedPositions) {
        const auto &e = entries[pos];
        double tfidfScore = 0.0;
        if (!tokens.isEmpty()) {
            const QString payloadLower = e.payload.toLower();
            for (const QString &tok : tokens) {
                if (!payloadLower.contains(tok))
                    continue;
                tfidfScore += idf.value(tok, 1.0);
            }
        }
        const double recency = static_cast<double>(pos) / static_cast<double>(N);

        const qint64 t = parseTimestampMs(e.timestamp);
        int neighbours = 0;
        for (qint64 ts : seedTimes) {
            if (ts == 0 || t == 0)
                continue;
            if (std::llabs(ts - t) <= config.correlationWindowMs)
                ++neighbours;
        }
        const double corr = neighbours > 1 ? std::log1p(neighbours - 1) : 0.0;

        const double s = config.wTfIdf * tfidfScore
                       + config.wRecency * recency
                       + config.wCorrelation * corr;
        scored.append({ pos, s });
    }

    std::sort(scored.begin(), scored.end(),
              [](const Scored &a, const Scored &b) { return a.score > b.score; });

    // 3. MMR-style greedy pick honouring diversity penalty.
    QSet<QString> seenKeys;
    QSet<int> pickedPositions;
    pickedPositions.reserve(config.maxEntries);
    for (const Scored &s : scored) {
        if (pickedPositions.size() >= config.maxEntries)
            break;
        const QString key = diversityKey(entries[s.pos]);
        if (seenKeys.contains(key) && config.wDiversityPenalty > 0.0) {
            // skip duplicates of the same (category|apid|ctid)
            continue;
        }
        seenKeys.insert(key);
        pickedPositions.insert(s.pos);
        out.seedIndices.append(entries[s.pos].index);
    }

    // 4. Expand picked positions with ±window contiguous neighbours.
    QSet<int> expanded = pickedPositions;
    for (int pos : pickedPositions) {
        const int lo = std::max(0, pos - config.windowBefore);
        const int hi = std::min(N - 1, pos + config.windowAfter);
        for (int j = lo; j <= hi; ++j)
            expanded.insert(j);
    }

    QList<int> orderedPositions(expanded.begin(), expanded.end());
    std::sort(orderedPositions.begin(), orderedPositions.end());

    out.entries.reserve(orderedPositions.size());
    for (int pos : orderedPositions)
        out.entries.append(entries[pos]);

    out.diagnostic =
        QStringLiteral("tokens=%1 seeds=%2 picked=%3 expanded=%4 idfTerms=%5")
            .arg(tokens.size())
            .arg(seedPositions.size())
            .arg(pickedPositions.size())
            .arg(expanded.size())
            .arg(idf.size());
    return out;
}

} // namespace dltchat
