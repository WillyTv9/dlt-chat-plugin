#include "dltchat/ai_query_pipeline.h"

#include "dltchat/fibex_enricher.h"
#include "dltchat/hierarchical_summary_store.h"
#include "dltchat/temporal_correlator.h"

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QtConcurrent/QtConcurrent>

#include <exception>

Q_LOGGING_CATEGORY(lcAiPipe, "dltchat.aipipeline")

namespace dltchat {

namespace {

QString computeCacheKey(const QString &query,
                        const QVector<DltAnalyzerInterface::LogEntry> &ctx,
                        const QString &digest)
{
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(query.toUtf8());
    const int n = qMin(20, ctx.size());
    for (int i = 0; i < n; ++i)
        h.addData(QByteArray::number(ctx[i].index));
    h.addData(QByteArray::number(ctx.size()));
    if (!digest.isEmpty())
        h.addData(QByteArray::number(static_cast<qint64>(qHash(digest))));
    return QString::fromLatin1(h.result().toHex());
}

} // namespace

AiQueryPipeline::AiQueryPipeline(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<dltchat::AiQueryPipeline::Result>();
}

AiQueryPipeline::~AiQueryPipeline()
{
    cancel();
    if (m_future.isRunning())
        m_future.waitForFinished();
}

bool AiQueryPipeline::isBusy() const
{
    return m_busy.load();
}

void AiQueryPipeline::cancel()
{
    m_cancelled.store(true);
}

void AiQueryPipeline::executeAsync(const Request &request)
{
    if (m_busy.load()) {
        m_cancelled.store(true);
        if (m_future.isRunning())
            m_future.waitForFinished();
    }
    m_cancelled.store(false);
    m_busy.store(true);

    Request req = request;
    m_future = QtConcurrent::run([this, req = std::move(req)]() mutable {
        runPipeline(std::move(req));
    });
}

void AiQueryPipeline::runPipeline(Request req)
{
    try {
        if (req.snapshot.isEmpty()) {
            m_busy.store(false);
            emit failed(QStringLiteral("AI/Pipeline"),
                        QStringLiteral("empty snapshot; load a DLT file first"));
            return;
        }

        BudgetPlan budget = ContextBudgetPlanner::plan(
            req.query, req.provider, req.model, req.conversationHistoryChars);
        QString effectiveQuery = req.query;
        ContextBudgetPlanner::hasDeepPrefix(req.query, &effectiveQuery);

        if (m_cancelled.load()) {
            m_busy.store(false);
            emit failed(QStringLiteral("AI/Pipeline"), QStringLiteral("cancelled"));
            return;
        }

        EnhancedRetriever retriever;
        EnhancedRetriever::Config rcfg;
        rcfg.maxEntries = budget.maxEntriesHint;
        rcfg.windowBefore = 5;
        rcfg.windowAfter = 5;
        auto retrieved = retriever.extract(effectiveQuery,
                                           req.snapshot,
                                           req.invertedIndex,
                                           req.selectedIndices,
                                           rcfg);

        if (m_cancelled.load()) {
            m_busy.store(false);
            emit failed(QStringLiteral("AI/Pipeline"), QStringLiteral("cancelled"));
            return;
        }

        QVector<DltAnalyzerInterface::LogEntry> contextEntries = retrieved.entries;
        if (contextEntries.isEmpty()) {
            // Fall back to the most recent slice so the model always has
            // *something* to look at — matches the previous behaviour.
            const int takeN = qMin(req.snapshot.size(), 200);
            contextEntries = req.snapshot.mid(req.snapshot.size() - takeN, takeN);
        }

        if (req.fibex && req.fibex->isLoaded())
            req.fibex->enrichAll(contextEntries);

        QString temporalContext;
        {
            TemporalCorrelator correlator;
            TemporalCorrelator::CorrelationConfig tcfg;
            tcfg.windowMs = 50;
            tcfg.minEntriesPerWindow = 2;
            tcfg.maxCorrelations = 3;
            temporalContext = correlator.analyze(contextEntries, tcfg);
            if (!correlator.hasCorrelations())
                temporalContext.clear();
        }

        QString hierDigest;
        if (req.hierStore && req.hierStore->isReady())
            hierDigest = req.hierStore->compactDigest(budget.hierChars);

        QString combinedExtra;
        if (!req.userFilterContext.isEmpty())
            combinedExtra = req.userFilterContext;
        if (!temporalContext.isEmpty()) {
            if (!combinedExtra.isEmpty())
                combinedExtra += QLatin1Char('\n');
            combinedExtra += temporalContext;
        }

        Result out;
        out.query = effectiveQuery;
        out.contextEntries = std::move(contextEntries);
        out.hierarchicalDigest = std::move(hierDigest);
        out.extraContext = std::move(combinedExtra);
        out.budget = budget;
        out.cacheKey = computeCacheKey(effectiveQuery, out.contextEntries, out.hierarchicalDigest);
        out.diagnostic =
            QStringLiteral("retrieved=%1 digest_chars=%2 budget=[%3] retriever=[%4]")
                .arg(out.contextEntries.size())
                .arg(out.hierarchicalDigest.size())
                .arg(budget.diagnostic, retrieved.diagnostic);

        qCInfo(lcAiPipe) << "prepared query;" << out.diagnostic;
        m_busy.store(false);
        emit prepared(out);
    } catch (const std::exception &e) {
        m_busy.store(false);
        emit failed(QStringLiteral("AI/Pipeline"),
                    QStringLiteral("std::exception: %1 (query=\"%2\" snapshot=%3)")
                        .arg(QString::fromUtf8(e.what()), req.query)
                        .arg(req.snapshot.size()));
    } catch (...) {
        m_busy.store(false);
        emit failed(QStringLiteral("AI/Pipeline"),
                    QStringLiteral("unknown exception (query=\"%1\")").arg(req.query));
    }
}

} // namespace dltchat
