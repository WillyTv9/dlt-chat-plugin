#ifndef DLTCHAT_HIERARCHICAL_SUMMARY_STORE_H
#define DLTCHAT_HIERARCHICAL_SUMMARY_STORE_H

#include "analyzer_interface.h"

#include <QHash>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QVector>

namespace dltchat {

struct LogStatistics
{
    int totalEntries = 0;
    QHash<QString, int> byEcu;
    QHash<QString, int> byApid;
    QHash<QString, int> byCtid;
    QHash<QString, int> byLevel;
    QHash<QString, int> byCategory;
    QHash<QString, int> byDomain;
    qint64 firstTimestampMs = 0;
    qint64 lastTimestampMs = 0;
};

struct BlockSummary
{
    int firstIdx = -1;
    int lastIdx = -1;
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString summary;
    QHash<QString, int> levelCounts;
    QStringList topApids;
    QStringList topKeywords;
};

struct EcuSummary
{
    QString ecu;
    int total = 0;
    QStringList topEvents;
    QHash<QString, int> categoryDist;
};

struct TimeWindowSummary
{
    qint64 startMs = 0;
    qint64 endMs = 0;
    int entryCount = 0;
    QString synthesis;
};

/**
 * In-memory container for pre-computed log summaries at multiple granularities.
 * Populated by LogIngestionPipeline on file load; read by ContextBudgetPlanner
 * and DltLlmAnalyzerInterface when building AI prompts.
 *
 * Thread-safe: write once from worker thread (pipeline), then read from
 * multiple threads via mutex-protected accessors.
 */
class HierarchicalSummaryStore
{
public:
    void setStatistics(const LogStatistics &stats);
    LogStatistics statistics() const;

    void setBlocks(const QVector<BlockSummary> &blocks);
    QVector<BlockSummary> blocks() const;

    void setEcuSummaries(const QHash<QString, EcuSummary> &ecuMap);
    QHash<QString, EcuSummary> ecuSummaries() const;

    QVector<TimeWindowSummary> byTimeWindow(qint64 windowMs) const;

    /**
     * Produce a compact textual digest suitable for inclusion in an LLM prompt.
     * Truncates to `maxChars` (best effort), prioritising global stats > ECU
     * distribution > block highlights. Returns empty string if no data.
     */
    QString compactDigest(int maxChars) const;

    void clear();
    bool isReady() const;

private:
    mutable QMutex m_mutex;
    LogStatistics m_stats;
    QVector<BlockSummary> m_blocks;
    QHash<QString, EcuSummary> m_ecus;
    bool m_ready = false;
};

} // namespace dltchat

#endif
