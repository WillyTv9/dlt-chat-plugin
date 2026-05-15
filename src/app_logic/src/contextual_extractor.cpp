#include "dltchat/contextual_extractor.h"

#include <QRegularExpression>
#include <algorithm>

namespace dltchat {

QVector<DltAnalyzerInterface::LogEntry> ContextualExtractor::extractContext(
    const QString &query,
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QHash<QString, QSet<int>> &invertedIndex,
    const QList<int> &selectedIndices,
    const ContextConfig &config) const
{
    QList<int> relevant = findRelevantIndices(query, entries, invertedIndex, config);

    if (!selectedIndices.isEmpty())
    {
        for (int idx : selectedIndices)
        {
            if (!relevant.contains(idx))
                relevant.append(idx);
        }
    }

    if (relevant.isEmpty())
        return {};

    std::sort(relevant.begin(), relevant.end());

    QSet<int> contextSet;
    QHash<int, int> indexToPos;
    for (int i = 0; i < entries.size(); ++i)
        indexToPos[entries[i].index] = i;

    for (int idx : relevant)
    {
        auto it = indexToPos.constFind(idx);
        if (it == indexToPos.constEnd()) continue;
        int pos = it.value();

        int start = qMax(0, pos - config.windowBefore);
        int end = qMin(entries.size() - 1, pos + config.windowAfter);

        for (int j = start; j <= end; ++j)
        {
            if (entries[j].index >= 0)
                contextSet.insert(entries[j].index);
        }
    }

    if (contextSet.size() > config.maxEntries)
    {
        QList<int> sorted = QList<int>(contextSet.begin(), contextSet.end());
        std::sort(sorted.begin(), sorted.end());

        int step = sorted.size() / config.maxEntries;
        QSet<int> sampled;
        for (int i = 0; i < sorted.size() && sampled.size() < config.maxEntries; i += qMax(1, step))
            sampled.insert(sorted[i]);

        contextSet = sampled;
    }

    QVector<DltAnalyzerInterface::LogEntry> result;
    result.reserve(contextSet.size());
    for (const auto &entry : entries)
    {
        if (contextSet.contains(entry.index))
            result.append(entry);
    }

    std::sort(result.begin(), result.end(),
        [](const auto &a, const auto &b) { return a.index < b.index; });

    return result;
}

QList<int> ContextualExtractor::findRelevantIndices(
    const QString &query,
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QHash<QString, QSet<int>> &invertedIndex,
    const ContextConfig &config) const
{
    Q_UNUSED(entries)
    Q_UNUSED(config)

    QStringList tokens = query.toLower().split(
        QRegularExpression("\\W+"), Qt::SkipEmptyParts);

    QSet<int> candidates;
    bool first = true;

    for (const QString &token : tokens)
    {
        if (token.size() < 3) continue;

        auto it = invertedIndex.constFind(token);
        if (it == invertedIndex.constEnd()) continue;

        if (first)
        {
            candidates = it.value();
            first = false;
        }
        else
        {
            candidates.intersect(it.value());
        }

        if (candidates.isEmpty()) break;
    }

    QList<int> result(candidates.begin(), candidates.end());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace dltchat
