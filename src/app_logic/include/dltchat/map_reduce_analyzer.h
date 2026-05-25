#ifndef DLTCHAT_MAP_REDUCE_ANALYZER_H
#define DLTCHAT_MAP_REDUCE_ANALYZER_H

#include "analyzer_interface.h"
#include "hierarchical_summary_store.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>

namespace dltchat {

class DltLlmAnalyzerInterface;

/**
 * Fan-out / fan-in analyser for "global" queries that need the model
 * to consider every part of a multi-million-entry log.
 *
 * Pipeline:
 *   1. Split the full log into N shards aligned with the blocks already
 *      computed by LogIngestionPipeline (default block size 5000 → 200
 *      shards per million entries).
 *   2. For each shard, fire one map-step LLM request via the shared
 *      DltLlmAnalyzerInterface, gated by ModelProfile.maxConcurrent
 *      so e.g. Copilot stays at 2 concurrent (single OAuth bearer +
 *      tight upstream limits).
 *   3. When all map shards return, build a reduce prompt that
 *      concatenates their condensed outputs and fire one final
 *      LLM request; its result is emitted as reduceReady().
 *
 * Reuses the existing TokenBucket + cache + retry/circuit-breaker
 * already implemented in DltLlmAnalyzerInterface — no new HTTP
 * client, no second OAuth.
 *
 * Map / reduce requests are distinguished from regular single-shot
 * traffic via a marker prefix (`<<MR:nonce:...>>`) inserted in the
 * originalQuery the analyser echoes back. Requests for other paths
 * remain visible to plugin_entry as before.
 */
class MapReduceAnalyzer : public QObject
{
    Q_OBJECT
public:
    struct Config
    {
        QString provider; // e.g. "copilot"
        QString model;    // e.g. "gpt-4o"
        int maxShards = 32;
        int mapMaxTokens = 600;
        int reduceMaxTokens = 1500;
        bool isGlobalQuery = true;
    };

    explicit MapReduceAnalyzer(QObject *parent = nullptr);

    void setLlmAnalyzer(DltLlmAnalyzerInterface *llm);

    /**
     * Kick off a map-reduce job. Returns immediately. Emits
     * shardCompleted as each shard returns, then reduceReady (or
     * failed) at the end. Only one job in flight at a time per
     * instance.
     */
    bool runAsync(const QString &userQuery,
                  const QVector<DltAnalyzerInterface::LogEntry> &allEntries,
                  const QVector<BlockSummary> &blocks,
                  const Config &config);

    bool isRunning() const;
    void cancel();

signals:
    void shardCompleted(int shardIdx, int total, qint64 elapsedMs);
    void reduceReady(const DltAnalyzerInterface::QueryResult &result,
                     const QString &originalQuery);
    void failed(const QString &stage, const QString &reason);

private slots:
    void onLlmResultReady(const DltAnalyzerInterface::QueryResult &result,
                          const QString &originalQuery);

private:
    struct ShardState
    {
        int idx = 0;
        int firstPos = 0;
        int lastPos = 0;
        QString markerQuery;
        QString output;
        bool done = false;
        qint64 launchedAtMs = 0;
    };

    void launchNextShards();
    void launchReduce();
    void finishWithError(const QString &stage, const QString &reason);

    DltLlmAnalyzerInterface *m_llm = nullptr;
    bool m_connected = false;

    Config m_config;
    QString m_userQuery;
    QString m_nonce;
    QVector<DltAnalyzerInterface::LogEntry> m_allEntries;
    QVector<ShardState> m_shards;
    QHash<QString, int> m_markerToShardIdx;
    QString m_reduceMarker;

    int m_nextToLaunch = 0;
    int m_inFlight = 0;
    int m_completed = 0;
    bool m_running = false;
    bool m_cancelled = false;
    bool m_reduceLaunched = false;
};

} // namespace dltchat

#endif
