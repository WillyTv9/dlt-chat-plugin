#ifndef DLTCHAT_AI_QUERY_PIPELINE_H
#define DLTCHAT_AI_QUERY_PIPELINE_H

#include "analyzer_interface.h"
#include "context_budget_planner.h"
#include "enhanced_retriever.h"

#include <QFuture>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

#include <atomic>

namespace dltchat {

class FibexEnricher;
class HierarchicalSummaryStore;

/**
 * Off-main-thread orchestrator that prepares everything an AI query
 * needs — retrieval, enrichment, temporal correlation, budget
 * planning — so the UI thread never blocks.
 *
 * Today the same work happens inline on the GUI thread inside
 * DltChatPlugin::handleAiQuery (extract context, enrich fibex,
 * correlate, hash cache key). On million-row logs that path freezes
 * the DLT Viewer window for seconds. This pipeline moves it to
 * QtConcurrent::run and hands the result back via a queued signal.
 *
 * The pipeline is intentionally stateless: callers pass a snapshot
 * of the log and a pointer to the long-lived FibexEnricher /
 * HierarchicalSummaryStore (read-only access). Multiple concurrent
 * queries are not supported — the caller must serialise.
 */
class AiQueryPipeline : public QObject
{
    Q_OBJECT
public:
    struct Request
    {
        QString query;
        QString provider;
        QString model;
        QVector<DltAnalyzerInterface::LogEntry> snapshot;
        QHash<QString, QSet<int>> invertedIndex;
        QList<int> selectedIndices;
        const FibexEnricher *fibex = nullptr;
        const HierarchicalSummaryStore *hierStore = nullptr;
        QString userFilterContext; // active user filters formatted by caller
        int conversationHistoryChars = 0;
    };

    struct Result
    {
        QString query;
        QVector<DltAnalyzerInterface::LogEntry> contextEntries;
        QString hierarchicalDigest;
        QString extraContext; // user filters + temporal correlations
        BudgetPlan budget;
        QString cacheKey;
        QString diagnostic;
    };

    explicit AiQueryPipeline(QObject *parent = nullptr);
    ~AiQueryPipeline() override;

    /**
     * Kick off a single query preparation. Emits prepared() with a
     * fully-populated Result on success or failed(stage, reason) on
     * error. Both signals are queued so the receiver runs on the GUI
     * thread.
     *
     * Safe to call again only after prepared/failed has fired. If
     * called while busy, the previous job is cancelled first.
     */
    void executeAsync(const Request &request);

    void cancel();
    bool isBusy() const;

signals:
    void prepared(const dltchat::AiQueryPipeline::Result &result);
    void failed(const QString &stage, const QString &reason);

private:
    void runPipeline(Request req);

    std::atomic<bool> m_cancelled{ false };
    std::atomic<bool> m_busy{ false };
    QFuture<void> m_future;
};

} // namespace dltchat

Q_DECLARE_METATYPE(dltchat::AiQueryPipeline::Result)

#endif
