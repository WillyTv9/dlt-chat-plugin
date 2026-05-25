#include "dltchat/map_reduce_analyzer.h"

#include "dltchat/llm_analyzer_interface.h"
#include "dltchat/model_profile_registry.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QRandomGenerator>
#include <QStringList>

#include <algorithm>

Q_LOGGING_CATEGORY(lcMr, "dltchat.mapreduce")

namespace dltchat {

MapReduceAnalyzer::MapReduceAnalyzer(QObject *parent)
    : QObject(parent)
{
}

void MapReduceAnalyzer::setLlmAnalyzer(DltLlmAnalyzerInterface *llm)
{
    if (m_llm == llm)
        return;
    if (m_llm && m_connected) {
        disconnect(m_llm, &DltLlmAnalyzerInterface::queryResultReady,
                   this, &MapReduceAnalyzer::onLlmResultReady);
        m_connected = false;
    }
    m_llm = llm;
    if (m_llm) {
        connect(m_llm, &DltLlmAnalyzerInterface::queryResultReady,
                this, &MapReduceAnalyzer::onLlmResultReady,
                Qt::DirectConnection);
        m_connected = true;
    }
}

bool MapReduceAnalyzer::isRunning() const
{
    return m_running;
}

void MapReduceAnalyzer::cancel()
{
    if (!m_running)
        return;
    m_cancelled = true;
    m_running = false;
    emit failed(QStringLiteral("MAPREDUCE"), QStringLiteral("cancelled by caller"));
}

bool MapReduceAnalyzer::runAsync(const QString &userQuery,
                                 const QVector<DltAnalyzerInterface::LogEntry> &allEntries,
                                 const QVector<BlockSummary> &blocks,
                                 const Config &config)
{
    if (m_running) {
        emit failed(QStringLiteral("MAPREDUCE"),
                    QStringLiteral("previous map-reduce job still running; refuse to overlap"));
        return false;
    }
    if (!m_llm) {
        emit failed(QStringLiteral("MAPREDUCE"),
                    QStringLiteral("DltLlmAnalyzerInterface not injected"));
        return false;
    }
    if (allEntries.isEmpty() || blocks.isEmpty()) {
        emit failed(QStringLiteral("MAPREDUCE"),
                    QStringLiteral("empty input (entries=%1 blocks=%2)")
                        .arg(allEntries.size()).arg(blocks.size()));
        return false;
    }

    m_config = config;
    m_userQuery = userQuery;
    m_allEntries = allEntries;
    m_cancelled = false;
    m_reduceLaunched = false;
    m_nextToLaunch = 0;
    m_inFlight = 0;
    m_completed = 0;
    m_shards.clear();
    m_markerToShardIdx.clear();

    m_nonce = QString::number(QRandomGenerator::global()->generate(), 16)
              + QString::number(QDateTime::currentMSecsSinceEpoch(), 16);

    // Map block indices to absolute positions in allEntries by binary
    // search over .index. blocks were produced from the same snapshot
    // by LogIngestionPipeline so the assumption holds.
    QHash<int, int> indexToPos;
    indexToPos.reserve(allEntries.size());
    for (int i = 0; i < allEntries.size(); ++i)
        indexToPos.insert(allEntries[i].index, i);

    QVector<BlockSummary> useBlocks = blocks;
    if (useBlocks.size() > config.maxShards) {
        const int factor = (useBlocks.size() + config.maxShards - 1) / config.maxShards;
        QVector<BlockSummary> merged;
        merged.reserve(config.maxShards);
        for (int i = 0; i < useBlocks.size(); i += factor) {
            BlockSummary b = useBlocks[i];
            const int last = std::min(useBlocks.size() - 1, i + factor - 1);
            b.lastIdx = useBlocks[last].lastIdx;
            b.endMs = useBlocks[last].endMs;
            merged.append(b);
        }
        useBlocks = merged;
    }

    m_shards.reserve(useBlocks.size());
    for (int i = 0; i < useBlocks.size(); ++i) {
        ShardState s;
        s.idx = i;
        s.firstPos = indexToPos.value(useBlocks[i].firstIdx, -1);
        s.lastPos = indexToPos.value(useBlocks[i].lastIdx, -1);
        if (s.firstPos < 0 || s.lastPos < 0 || s.lastPos < s.firstPos)
            continue;
        if (config.isGlobalQuery) {
            s.markerQuery = QStringLiteral("<<MR:%1:%2/%3>> %4")
                                .arg(m_nonce)
                                .arg(i)
                                .arg(useBlocks.size())
                                .arg(userQuery);
        } else {
            // For specific queries, instruct each shard to find and list
            // relevant entries with their indices for the reduce step.
            s.markerQuery = QStringLiteral("<<MR:%1:%2/%3>> [SHARD %2/%3: examine these log entries and list those RELEVANT to the query below. For each relevant entry, reference it as [index:N]. If none in this shard, say \"no relevant entries in this shard\".]\n%4")
                                .arg(m_nonce)
                                .arg(i)
                                .arg(useBlocks.size())
                                .arg(userQuery);
        }
        m_markerToShardIdx.insert(s.markerQuery, s.idx);
        m_shards.append(s);
    }

    m_reduceMarker = QStringLiteral("<<MR:%1:reduce>> %2").arg(m_nonce, userQuery);

    m_running = true;
    qCInfo(lcMr) << "starting map-reduce; shards=" << m_shards.size()
                 << "concurrency=" << ModelProfileRegistry::profileFor(config.provider, config.model).maxConcurrent
                 << "nonce=" << m_nonce;

    launchNextShards();
    return true;
}

void MapReduceAnalyzer::launchNextShards()
{
    const ModelProfile prof = ModelProfileRegistry::profileFor(m_config.provider, m_config.model);
    const int cap = std::max(1, prof.maxConcurrent);

    while (m_inFlight < cap && m_nextToLaunch < m_shards.size() && !m_cancelled) {
        ShardState &s = m_shards[m_nextToLaunch++];
        s.launchedAtMs = QDateTime::currentMSecsSinceEpoch();

        QVector<DltAnalyzerInterface::LogEntry> slice(
            m_allEntries.begin() + s.firstPos,
            m_allEntries.begin() + s.lastPos + 1);

        const int prevMax = m_llm->maxTokens();
        m_llm->setMaxTokens(m_config.mapMaxTokens);
        const bool ok = m_llm->analyzeQueryAsync(s.markerQuery, slice);
        m_llm->setMaxTokens(prevMax);
        if (!ok) {
            finishWithError(QStringLiteral("MAPREDUCE/shard %1").arg(s.idx),
                            QStringLiteral("analyzeQueryAsync returned false (in-progress or unavailable)"));
            return;
        }
        ++m_inFlight;
        qCInfo(lcMr) << "launched shard" << s.idx << "of" << m_shards.size()
                     << "entries=" << slice.size();
    }
}

void MapReduceAnalyzer::onLlmResultReady(const DltAnalyzerInterface::QueryResult &result,
                                         const QString &originalQuery)
{
    if (!m_running)
        return;

    if (originalQuery == m_reduceMarker) {
        m_running = false;
        QString cleanedOriginal = m_userQuery;
        emit reduceReady(result, cleanedOriginal);
        return;
    }

    auto it = m_markerToShardIdx.find(originalQuery);
    if (it == m_markerToShardIdx.end())
        return; // not our event — leave for the normal single-shot handler

    const int idx = it.value();
    if (idx < 0 || idx >= m_shards.size())
        return;

    ShardState &s = m_shards[idx];
    if (s.done)
        return;
    s.done = true;
    --m_inFlight;
    ++m_completed;

    if (!result.success) {
        finishWithError(QStringLiteral("MAPREDUCE/shard %1").arg(s.idx),
                        QStringLiteral("LLM failed: %1").arg(result.errorMessage));
        return;
    }
    // Strip any HTML the analyser may have wrapped around the answer.
    QString plain = result.responseHtml;
    plain.replace(QRegExp("<[^>]+>"), QStringLiteral(" "));
    s.output = plain.simplified().left(2000);

    const qint64 ms = QDateTime::currentMSecsSinceEpoch() - s.launchedAtMs;
    emit shardCompleted(s.idx, m_shards.size(), ms);

    if (m_completed == m_shards.size()) {
        launchReduce();
        return;
    }
    launchNextShards();
}

void MapReduceAnalyzer::launchReduce()
{
    if (m_reduceLaunched)
        return;
    m_reduceLaunched = true;

    QStringList parts;
    parts.reserve(m_shards.size());
    for (const ShardState &s : m_shards)
        parts << QStringLiteral("[shard %1/%2] %3").arg(s.idx + 1).arg(m_shards.size()).arg(s.output);

    const QString combined = parts.join(QLatin1Char('\n'));
    m_llm->setExtraContext(QStringLiteral("MAP_STEP_OUTPUTS (%1 shards):\n%2")
                               .arg(m_shards.size())
                               .arg(combined));

    // Pass a single synthetic entry so buildEnhancedPrompt does not
    // short-circuit on empty input; the real payload is in extraContext.
    DltAnalyzerInterface::LogEntry synthetic;
    synthetic.index = -1;
    synthetic.payload = QStringLiteral("Map-reduce reduce step: synthesise per-shard summaries above.");
    QVector<DltAnalyzerInterface::LogEntry> single{ synthetic };

    const int prevMax = m_llm->maxTokens();
    m_llm->setMaxTokens(m_config.reduceMaxTokens);
    const bool ok = m_llm->analyzeQueryAsync(m_reduceMarker, single);
    m_llm->setMaxTokens(prevMax);
    if (!ok) {
        finishWithError(QStringLiteral("MAPREDUCE/reduce"),
                        QStringLiteral("reduce analyzeQueryAsync returned false"));
        return;
    }
    qCInfo(lcMr) << "launched reduce step; shards consolidated=" << m_shards.size();
}

void MapReduceAnalyzer::finishWithError(const QString &stage, const QString &reason)
{
    m_running = false;
    m_cancelled = true;
    qCWarning(lcMr) << stage << reason;
    emit failed(stage, reason);
}

} // namespace dltchat
