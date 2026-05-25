#include "dltchat/log_ingestion_pipeline.h"

#include "dltchat/analyzer_interface.h"
#include "dltchat/log_store.h"

#include <QHash>
#include <QHashIterator>
#include <QLoggingCategory>
#include <QMap>
#include <QString>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <exception>

Q_LOGGING_CATEGORY(lcIngest, "dltchat.ingestion")

namespace dltchat {

namespace {

qint64 parseTimestampMs(const QString &ts)
{
    if (ts.isEmpty())
        return 0;
    if (ts.contains(QLatin1Char(':'))) {
        QStringList parts = ts.split(QLatin1Char(':'));
        if (parts.size() == 3) {
            qint64 h = parts[0].toLongLong();
            qint64 m = parts[1].toLongLong();
            double s = parts[2].toDouble();
            return (h * 3600 + m * 60) * 1000 + static_cast<qint64>(s * 1000.0);
        }
    }
    bool ok = false;
    double sec = ts.toDouble(&ok);
    if (ok)
        return static_cast<qint64>(sec * 1000.0);
    return ts.toLongLong();
}

QStringList topNByCount(const QHash<QString, int> &h, int n)
{
    QVector<QPair<QString, int>> v;
    v.reserve(h.size());
    for (auto it = h.constBegin(); it != h.constEnd(); ++it)
        v.append({ it.key(), it.value() });
    std::sort(v.begin(), v.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    QStringList out;
    const int k = std::min(n, v.size());
    for (int i = 0; i < k; ++i)
        out << v[i].first;
    return out;
}

} // namespace

LogIngestionPipeline::LogIngestionPipeline(QObject *parent)
    : QObject(parent)
{
}

LogIngestionPipeline::~LogIngestionPipeline()
{
    cancel();
    if (m_future.isRunning())
        m_future.waitForFinished();
}

void LogIngestionPipeline::setSummaryStore(HierarchicalSummaryStore *store)
{
    m_store = store;
}

void LogIngestionPipeline::setRuleBasedAnalyzer(DltRuleBasedAnalyzer *analyzer)
{
    m_rules = analyzer;
}

void LogIngestionPipeline::setBlockSize(int blockSize)
{
    if (blockSize > 0)
        m_blockSize = blockSize;
}

int LogIngestionPipeline::blockSize() const
{
    return m_blockSize;
}

bool LogIngestionPipeline::isReady() const
{
    return m_ready.load();
}

void LogIngestionPipeline::cancel()
{
    m_cancelled.store(true);
}

void LogIngestionPipeline::startAsync(LogStore *store)
{
    if (!store) {
        emit failed(QStringLiteral("INGESTION"),
                    QStringLiteral("LogStore pointer is null"));
        return;
    }
    startAsync(store->copyAll());
}

void LogIngestionPipeline::startAsync(QVector<DltAnalyzerInterface::LogEntry> snapshot)
{
    if (!m_store) {
        emit failed(QStringLiteral("INGESTION"),
                    QStringLiteral("HierarchicalSummaryStore not injected; call setSummaryStore() before startAsync()"));
        return;
    }

    if (m_running.load()) {
        m_cancelled.store(true);
        if (m_future.isRunning())
            m_future.waitForFinished();
    }

    m_cancelled.store(false);
    m_ready.store(false);
    m_running.store(true);

    qCInfo(lcIngest) << "starting pipeline; entries=" << snapshot.size()
                     << "blockSize=" << m_blockSize;

    m_future = QtConcurrent::run([this, snap = std::move(snapshot)]() mutable {
        runPipeline(std::move(snap));
    });
}

void LogIngestionPipeline::runPipeline(QVector<DltAnalyzerInterface::LogEntry> snapshot)
{
    try {
        if (snapshot.isEmpty()) {
            m_ready.store(true);
            m_running.store(false);
            emit ready();
            return;
        }

        LogStatistics stats;
        QHash<QString, EcuSummary> ecuMap;
        runStageA(snapshot, stats, ecuMap);
        if (m_cancelled.load()) {
            m_running.store(false);
            emit failed(QStringLiteral("INGESTION/StageA"), QStringLiteral("cancelled"));
            return;
        }
        m_store->setStatistics(stats);
        m_store->setEcuSummaries(ecuMap);
        emit stageProgress(QStringLiteral("StageA"), 100);

        QVector<BlockSummary> blocks;
        runStageB(snapshot, blocks);
        if (m_cancelled.load()) {
            m_running.store(false);
            emit failed(QStringLiteral("INGESTION/StageB"), QStringLiteral("cancelled"));
            return;
        }
        m_store->setBlocks(blocks);
        emit stageProgress(QStringLiteral("StageB"), 100);

        m_ready.store(true);
        m_running.store(false);
        qCInfo(lcIngest) << "pipeline done; blocks=" << blocks.size();
        emit ready();
    } catch (const std::exception &e) {
        m_running.store(false);
        emit failed(QStringLiteral("INGESTION"),
                    QStringLiteral("std::exception: %1 (entries=%2 blockSize=%3)")
                        .arg(QString::fromUtf8(e.what()))
                        .arg(snapshot.size())
                        .arg(m_blockSize));
    } catch (...) {
        m_running.store(false);
        emit failed(QStringLiteral("INGESTION"),
                    QStringLiteral("unknown exception (entries=%1)").arg(snapshot.size()));
    }
}

void LogIngestionPipeline::runStageA(const QVector<DltAnalyzerInterface::LogEntry> &snapshot,
                                     LogStatistics &outStats,
                                     QHash<QString, EcuSummary> &outEcuMap)
{
    outStats = {};
    outStats.totalEntries = snapshot.size();

    qint64 firstMs = 0;
    qint64 lastMs = 0;
    bool firstSeen = false;

    const int notifyEvery = std::max(1, snapshot.size() / 20);
    int counter = 0;

    for (const auto &e : snapshot) {
        if (m_cancelled.load())
            return;

        if (!e.ecu.isEmpty()) {
            outStats.byEcu[e.ecu]++;
            auto &es = outEcuMap[e.ecu];
            if (es.ecu.isEmpty())
                es.ecu = e.ecu;
            es.total++;
            if (!e.category.isEmpty())
                es.categoryDist[e.category]++;
            if (!e.event.isEmpty() && !es.topEvents.contains(e.event)
                && es.topEvents.size() < 10)
                es.topEvents << e.event;
        }
        if (!e.apid.isEmpty())
            outStats.byApid[e.apid]++;
        if (!e.ctid.isEmpty())
            outStats.byCtid[e.ctid]++;
        if (!e.level.isEmpty())
            outStats.byLevel[e.level]++;
        if (!e.category.isEmpty())
            outStats.byCategory[e.category]++;
        if (!e.domain.isEmpty())
            outStats.byDomain[e.domain]++;

        const qint64 ts = parseTimestampMs(e.timestamp);
        if (ts > 0) {
            if (!firstSeen) {
                firstMs = ts;
                lastMs = ts;
                firstSeen = true;
            } else {
                firstMs = std::min(firstMs, ts);
                lastMs = std::max(lastMs, ts);
            }
        }
        if (++counter % notifyEvery == 0)
            emit stageProgress(QStringLiteral("StageA"),
                               static_cast<int>(100.0 * counter / snapshot.size()));
    }

    outStats.firstTimestampMs = firstMs;
    outStats.lastTimestampMs = lastMs;
}

void LogIngestionPipeline::runStageB(const QVector<DltAnalyzerInterface::LogEntry> &snapshot,
                                     QVector<BlockSummary> &outBlocks)
{
    const int N = snapshot.size();
    const int bs = m_blockSize;
    const int totalBlocks = (N + bs - 1) / bs;
    outBlocks.reserve(totalBlocks);

    for (int b = 0; b < totalBlocks; ++b) {
        if (m_cancelled.load())
            return;

        const int lo = b * bs;
        const int hi = std::min(N, lo + bs);

        BlockSummary blk;
        blk.firstIdx = snapshot[lo].index;
        blk.lastIdx = snapshot[hi - 1].index;
        blk.startMs = parseTimestampMs(snapshot[lo].timestamp);
        blk.endMs = parseTimestampMs(snapshot[hi - 1].timestamp);

        QHash<QString, int> apidCount;
        QHash<QString, int> termCount;
        for (int i = lo; i < hi; ++i) {
            const auto &e = snapshot[i];
            if (!e.level.isEmpty())
                blk.levelCounts[e.level]++;
            if (!e.apid.isEmpty())
                apidCount[e.apid]++;
            if (!e.category.isEmpty())
                termCount[e.category]++;
        }
        blk.topApids = topNByCount(apidCount, 3);
        blk.topKeywords = topNByCount(termCount, 5);

        if (m_rules) {
            // Use the rule-based analyzer's deterministic synthesis as the
            // per-block summary. Pass a slice; the analyzer treats it as
            // a standalone query result and returns ~one line of HTML.
            QVector<DltAnalyzerInterface::LogEntry> slice(snapshot.begin() + lo,
                                                          snapshot.begin() + hi);
            auto qr = m_rules->analyzeQuery(QStringLiteral("summary"), slice);
            // Strip the heaviest HTML to keep the digest compact.
            QString plain = qr.responseHtml;
            plain.replace(QRegExp("<[^>]+>"), QStringLiteral(" "));
            plain = plain.simplified();
            blk.summary = plain.left(200);
        }

        outBlocks.append(blk);
        if (b % std::max(1, totalBlocks / 20) == 0)
            emit stageProgress(QStringLiteral("StageB"),
                               static_cast<int>(100.0 * (b + 1) / totalBlocks));
    }
}

} // namespace dltchat
