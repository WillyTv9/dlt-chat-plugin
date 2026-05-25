#ifndef DLTCHAT_LOG_INGESTION_PIPELINE_H
#define DLTCHAT_LOG_INGESTION_PIPELINE_H

#include "analyzer_interface.h"
#include "hierarchical_summary_store.h"

#include <QFuture>
#include <QObject>
#include <QString>
#include <QVector>

#include <atomic>

namespace dltchat {

class LogStore;
class DltRuleBasedAnalyzer;

/**
 * Off-main-thread orchestrator that, on file load, walks the LogStore
 * once and populates a HierarchicalSummaryStore with:
 *
 *   Stage A — global statistics (count by ECU/APID/CTID/level/category/
 *             domain, timestamp span). Cheap; ~5s on 1M entries.
 *
 *   Stage B — block summaries (contiguous blocks of `blockSize` entries
 *             each get a deterministic summary from the existing
 *             DltRuleBasedAnalyzer, plus level histogram and topApids).
 *
 * Runs entirely off the UI thread via QtConcurrent::run. UI may be used
 * (with the rule-based fallback) before `ready()` arrives — the AI
 * dispatcher just checks `isReady()` and degrades gracefully.
 *
 * No new UI widgets: progress reaches the host plugin via signals and
 * goes to qInfo() logs. The UI shows AI output as before.
 */
class LogIngestionPipeline : public QObject
{
    Q_OBJECT
public:
    explicit LogIngestionPipeline(QObject *parent = nullptr);
    ~LogIngestionPipeline() override;

    /** Inject the store that Stage A/B will populate. Mandatory. */
    void setSummaryStore(HierarchicalSummaryStore *store);

    /** Optional: enables Stage B summary generation (otherwise blocks are
     *  populated with metadata only and an empty summary string). */
    void setRuleBasedAnalyzer(DltRuleBasedAnalyzer *analyzer);

    /** Configure the contiguous block size for Stage B. Default 5000 — good
     *  trade-off between digest detail and pipeline runtime on 1M-row files. */
    void setBlockSize(int blockSize);
    int blockSize() const;

    /** Snapshot the LogStore and process it. Returns immediately; emits
     *  stageProgress / ready / failed via queued connections. Safe to call
     *  more than once: a running pipeline is cancelled first. */
    void startAsync(LogStore *store);

    /** Same as above but accepts an already-prepared entry snapshot.
     *  Use this when the plugin keeps its own QVector (current case in
     *  plugin_entry.cpp) instead of a LogStore instance. */
    void startAsync(QVector<DltAnalyzerInterface::LogEntry> snapshot);

    /** Cooperative cancellation — atomic flag checked between blocks. */
    void cancel();

    /** True once a previous startAsync() has finished and populated the store. */
    bool isReady() const;

signals:
    void stageProgress(const QString &stage, int pctComplete);
    void ready();
    /** Detailed, technician-grade error string. Caller routes to UI via
     *  AiErrorReporter (separate commit). */
    void failed(const QString &stage, const QString &reason);

private:
    void runPipeline(QVector<DltAnalyzerInterface::LogEntry> snapshot);
    void runStageA(const QVector<DltAnalyzerInterface::LogEntry> &snapshot,
                   LogStatistics &outStats,
                   QHash<QString, EcuSummary> &outEcuMap);
    void runStageB(const QVector<DltAnalyzerInterface::LogEntry> &snapshot,
                   QVector<BlockSummary> &outBlocks);

    HierarchicalSummaryStore *m_store = nullptr;
    DltRuleBasedAnalyzer *m_rules = nullptr;
    int m_blockSize = 5000;
    std::atomic<bool> m_cancelled{ false };
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_ready{ false };
    QFuture<void> m_future;
};

} // namespace dltchat

#endif
