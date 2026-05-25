#include "plugin_entry.h"
#include "dltaioptionsdialog.h"
#include "dltchat/temporal_correlator.h"
#include "dltchat/category_registry.h"
#include "dltchat_version.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QRegularExpression>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QMutexLocker>
#include <QSettings>
#include <QThread>
#include <algorithm>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QtConcurrent>

using namespace dltchat;

static constexpr int kAIPreFilterMax = 100000;
static constexpr int kSnippetLength = 120;
static constexpr int kPayloadTruncateAt = 500;
static constexpr int kPreviewCount = 20;
static constexpr int kAICacheMaxEntries = 10000;
static constexpr int kAIDebounceMs = 500;

static const char *STOPWORDS[] = {
    "the", "and", "this", "that", "what", "which", "with", "from", "have", "been",
    "show", "mostra", "elenca", "tutti", "tutte", "all", "why", "perche", "causa",
    "motivo", "summary", "summarize", "riassumi", "sintesi", "log", "logs",
    "messaggi", "messaggio", "indice", "index", "riga", "line", "timestamp", "time",
    "tempo", "error", "errors", "errore", "errori", "fatal", "fatale", "warn",
    "warning", "avviso", "info", "debug", "verbose", "context", "before", "after",
    "can", "not", "are", "was", "for", "will", "has", "had", "but", "its", "also"
};

DltChatPlugin::DltChatPlugin()
    : form(nullptr), dltFile(nullptr), mainTableView(nullptr), messageDecoder(nullptr)
    , highlightColor(255, 230, 128)
    , m_analyzer(nullptr), m_ruleBasedAnalyzer(nullptr), m_llmAnalyzer(nullptr)
    , m_currentAnalyzerType("rule-based")
    , m_bulkAnalyzer(nullptr)
    , m_bulkAnalysisEnabled(false)
    , m_bulkAnalysisInProgress(false)
    , m_aiAvailabilityRetryCount(0)
{
    indexStopwords.reserve(70);
    for (const char *w : STOPWORDS)
        indexStopwords.insert(QString::fromLatin1(w));
    setupDefaultAnalyzer();

    m_userFilterManager = new UserFilterManager(this);
    connect(m_userFilterManager, &UserFilterManager::filtersChanged,
            this, [this]() {
                applyUserFilterHighlights();
                updateDomainStatus();
            });

    m_bulkAnalyzer = new DltBulkAnalyzer(this);
    m_bulkAnalyzer->setAnalyzer(m_llmAnalyzer);
    connect(m_bulkAnalyzer, &DltBulkAnalyzer::progressUpdated,
            this, &DltChatPlugin::onBulkProgress);
    connect(m_bulkAnalyzer, &DltBulkAnalyzer::analysisFinished,
            this, &DltChatPlugin::onBulkFinished);
    connect(m_bulkAnalyzer, &DltBulkAnalyzer::errorOccurred,
            this, &DltChatPlugin::onBulkError);

    m_aiHealthTimer = new QTimer(this);
    m_aiHealthTimer->setInterval(60000);
    connect(m_aiHealthTimer, &QTimer::timeout, this, &DltChatPlugin::onAiHealthCheck);

    // --- Off-main-thread AI pipeline (replaces the inline retrieval/enrich/
    // correlate that used to freeze the GUI on million-row logs). Wired now;
    // exercised by initFileFinish() and onAiQuerySubmitted(). No UI widgets
    // added — progress and errors flow through the existing chat channel.
    m_ingestionPipeline = new dltchat::LogIngestionPipeline(this);
    m_ingestionPipeline->setSummaryStore(&m_hierStore);
    m_ingestionPipeline->setRuleBasedAnalyzer(m_ruleBasedAnalyzer);
    m_ingestionPipeline->setBlockSize(m_ingestBlockSize);
    connect(m_ingestionPipeline, &dltchat::LogIngestionPipeline::ready,
            this, &DltChatPlugin::onIngestionReady);
    connect(m_ingestionPipeline, &dltchat::LogIngestionPipeline::stageProgress,
            this, &DltChatPlugin::onIngestionStageProgress);
    connect(m_ingestionPipeline, &dltchat::LogIngestionPipeline::failed,
            this, &DltChatPlugin::onIngestionFailed);

    m_aiQueryPipeline = new dltchat::AiQueryPipeline(this);
    connect(m_aiQueryPipeline, &dltchat::AiQueryPipeline::prepared,
            this, &DltChatPlugin::onAiPipelinePrepared);
    connect(m_aiQueryPipeline, &dltchat::AiQueryPipeline::failed,
            this, &DltChatPlugin::onAiPipelineFailed);

    m_mapReduceAnalyzer = new dltchat::MapReduceAnalyzer(this);
    m_mapReduceAnalyzer->setLlmAnalyzer(m_llmAnalyzer);
    connect(m_mapReduceAnalyzer, &dltchat::MapReduceAnalyzer::shardCompleted,
            this, &DltChatPlugin::onMapReduceShardCompleted);
    connect(m_mapReduceAnalyzer, &dltchat::MapReduceAnalyzer::reduceReady,
            this, &DltChatPlugin::onMapReduceReduceReady);
    connect(m_mapReduceAnalyzer, &dltchat::MapReduceAnalyzer::failed,
            this, &DltChatPlugin::onMapReduceFailed);

    QTimer::singleShot(0, this, &DltChatPlugin::checkAiAvailabilityAsync);
}

DltChatPlugin::~DltChatPlugin()
{
    delete m_userFilterManager;
    delete m_bulkAnalyzer;
    delete m_llmAnalyzer;
    delete m_ruleBasedAnalyzer;
}

void DltChatPlugin::setupDefaultAnalyzer()
{
    m_ruleBasedAnalyzer = new DltRuleBasedAnalyzer();
    m_analyzer = m_ruleBasedAnalyzer;

    m_llmAnalyzer = DltLlmAnalyzerFactory::createOllamaAnalyzer(
        "http://localhost:11434", "llama3.2:1b", this);

    connect(m_llmAnalyzer, &DltLlmAnalyzerInterface::queryResultReady,
            this, &DltChatPlugin::onLlmResultReady);
}

void DltChatPlugin::startBulkAnalysis()
{
    if (!m_bulkAnalysisEnabled) return;
    if (m_bulkAnalyzer->isRunning()) return;

    QMutexLocker lock(&entriesMutex);
    if (entries.isEmpty()) return;

    m_bulkAnalysisInProgress = true;
    updateStatus(QString("Bulk analysis started for %1 logs...").arg(entries.size()));
    m_bulkAnalyzer->startBulkAnalysis(entries, 100);
}

void DltChatPlugin::onBulkProgress(double progress, int processed, int total)
{
    QString status = QString("Bulk analysis: %1/%2 (%3%)")
        .arg(processed)
        .arg(total)
        .arg(static_cast<int>(progress * 100));
    updateStatus(status);
}

void DltChatPlugin::onBulkFinished(bool success)
{
    m_bulkAnalysisInProgress = false;
    if (success)
    {
        updateStatus("Bulk analysis completed successfully.");
    }
    else
    {
        updateStatus("Bulk analysis finished with errors.");
    }
}

bool DltChatPlugin::loadFibexFile(const QString &filePath, QString *errorOut)
{
    bool ok = m_fibexEnricher.loadFile(filePath, errorOut);
    if (ok && form) {
        updateStatus(QString("Fibex loaded: %1 (%2 mappings)")
            .arg(filePath).arg(m_fibexEnricher.mappingCount()));
    }
    return ok;
}

void DltChatPlugin::onBulkError(const QString &error)
{
    m_bulkAnalysisInProgress = false;
    updateStatus(QString("Bulk analysis error: %1").arg(error));
}

static QString buildAiCacheKey(const QString &query, const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(query.toUtf8());
    int n = qMin(entries.size(), 20);
    for (int i = 0; i < n; ++i) {
        hash.addData(QByteArray::number(entries[i].index));
    }
    hash.addData(QByteArray::number(entries.size()));
    return hash.result().toHex();
}

void DltChatPlugin::checkAiAvailabilityAsync()
{
    if (!m_llmAnalyzer || !m_llmAnalyzer->validateConfiguration())
    {
        setAiState(0);
        return;
    }

    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QString provider = m_llmAnalyzer->detectProviderType();
    QNetworkRequest probeReq;

    if (provider == "copilot") {
        setAiState(2, m_llmAnalyzer->modelName());
        delete mgr;
        return;
    }

    if (provider == "ollama") {
        QString tagsUrl = m_llmAnalyzer->apiEndpoint();
        tagsUrl.replace("/api/generate", "/api/tags").replace("/api/chat", "/api/tags");
        probeReq.setUrl(QUrl(tagsUrl));
    } else {
        QUrl base(m_llmAnalyzer->apiEndpoint());
        QString modelsUrl = base.scheme() + "://" + base.authority() + "/v1/models";
        probeReq.setUrl(QUrl(modelsUrl));
        QString probeKey = m_llmAnalyzer->apiKey();
        if (!probeKey.isEmpty())
            probeReq.setRawHeader("Authorization",
                QString("Bearer %1").arg(probeKey).toUtf8());
    }

    QNetworkReply *reply = mgr->get(probeReq);
    connect(reply, &QNetworkReply::finished, this, [this, reply, mgr, provider]() {
        reply->deleteLater();
        mgr->deleteLater();

        if (provider == "ollama") {
            if (reply->error() == QNetworkReply::NoError)
            {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                if (doc.isObject())
                {
                    QJsonArray models = doc.object()["models"].toArray();
                    for (const auto &m : models)
                    {
                        QString name = m.toObject()["name"].toString();
                        if (name.contains(m_llmAnalyzer->modelName(), Qt::CaseInsensitive))
                        {
                            m_aiAvailabilityRetryCount = 0;
                            setAiState(2, m_llmAnalyzer->modelName());
                            return;
                        }
                    }
                    if (!models.isEmpty()) {
                        setAiState(2, m_llmAnalyzer->modelName());
                        return;
                    }
                }
            }
            setAiState(1, m_llmAnalyzer->modelName());
        } else {
            int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            bool ok = (httpCode >= 200 && httpCode < 300);
            setAiState(ok ? 2 : 1, m_llmAnalyzer->modelName());
        }
    });
}

void DltChatPlugin::setAiState(int state, const QString &modelName)
{
    m_aiState = state;
    if (state == 2) {
        m_aiModelName = modelName;
        m_aiAvailabilityRetryCount = 0;
    } else if (state == 1) {
        m_aiModelName = modelName.isEmpty() ? m_llmAnalyzer->modelName() : modelName;
        m_aiAvailabilityRetryCount++;
    } else {
        m_aiModelName.clear();
        m_aiAvailabilityRetryCount = 0;
    }
    m_aiAvailabilityTimer.start();

    if (m_aiHealthTimer) {
        if (state == 0)
            m_aiHealthTimer->stop();
        else if (!m_aiHealthTimer->isActive())
            m_aiHealthTimer->start();
    }

    emit onAiAvailabilityChanged(state, m_aiModelName);
    updateDomainStatus();
}

int DltChatPlugin::aiAvailabilityBackoffMs() const
{
    if (m_aiAvailabilityRetryCount <= 1) return 5000;
    if (m_aiAvailabilityRetryCount <= 3) return 15000;
    return 60000;
}

void DltChatPlugin::onAiHealthCheck()
{
    if (m_llmAnalyzer && m_llmAnalyzer->validateConfiguration())
        checkAiAvailabilityAsync();
}

QString DltChatPlugin::name() { return QString("Chat Log Assistant"); }
QString DltChatPlugin::pluginVersion() { return DLT_CHAT_PLUGIN_VERSION; }
QString DltChatPlugin::pluginInterfaceVersion() { return PLUGIN_INTERFACE_VERSION; }
QString DltChatPlugin::description()
{
    return QString("Chat-based log analysis for DLT Viewer with rule-based and AI support");
}
QString DltChatPlugin::error() { return errorText; }

bool DltChatPlugin::loadConfig(QString filename)
{
    errorText.clear();
    if (filename.isEmpty())
    {
        applyConfigToForm();
        return true;
    }
    QSettings settings(filename, QSettings::IniFormat);
    settings.beginGroup("Analyzer");
    QString t = settings.value("type", "rule-based").toString();
    setAnalyzerType(t);
    QString ep = settings.value("llmEndpoint", "").toString();
    QString key = settings.value("llmApiKey", "").toString();
    QString model = settings.value("llmModel", "llama3.2:1b").toString();
    m_copilotOAuthToken = settings.value("copilotOAuthToken", "").toString();
    m_copilotClientId = settings.value("copilotClientId", "").toString();
    int maxTokens = settings.value("llmMaxTokens", 4096).toInt();
    double temperature = settings.value("llmTemperature", 0.7).toDouble();
    int timeoutMs = settings.value("llmTimeout", 120000).toInt();
    if (!ep.isEmpty()) {
        if (key.isEmpty() && !m_copilotOAuthToken.isEmpty())
            key = m_copilotOAuthToken;
        configureLlmAnalyzer(ep, key, model);
        if (m_llmAnalyzer) {
            m_llmAnalyzer->setMaxTokens(maxTokens);
            m_llmAnalyzer->setTemperature(temperature);
            m_llmAnalyzer->setTimeout(timeoutMs);
        }
    }
    m_bulkAnalysisEnabled = settings.value("bulkAnalysisEnabled", false).toBool();
    settings.endGroup();
    settings.beginGroup("Behavior");
    if (settings.contains("highlightColor"))
        highlightColor = QColor(settings.value("highlightColor").toString());
    settings.endGroup();
    // Optional .dlp override; empty => embedded ":/dltchat/default_filters.dlp".
    settings.beginGroup("Filters");
    m_dlpFilterPath = settings.value("dlpPath", QString()).toString();
    settings.endGroup();
    loadNativeFilterCatalog();
    populateNativeFilterMenus();
    applyConfigToForm();
    return true;
}

bool DltChatPlugin::saveConfig(QString filename)
{
    if (filename.isEmpty()) return false;
    QSettings s(filename, QSettings::IniFormat);
    s.beginGroup("Analyzer");
    s.setValue("type", m_currentAnalyzerType);
    if (m_llmAnalyzer)
    {
        s.setValue("llmEndpoint", m_llmAnalyzer->apiEndpoint());
        s.setValue("llmApiKey", m_llmAnalyzer->apiKey());
        s.setValue("llmModel", m_llmAnalyzer->modelName());
        s.setValue("llmMaxTokens", m_llmAnalyzer->maxTokens());
        s.setValue("llmTemperature", m_llmAnalyzer->temperature());
        s.setValue("llmTimeout", m_llmAnalyzer->timeout());
    }
    if (!m_copilotOAuthToken.isEmpty())
        s.setValue("copilotOAuthToken", m_copilotOAuthToken);
    if (!m_copilotClientId.isEmpty())
        s.setValue("copilotClientId", m_copilotClientId);
    s.setValue("bulkAnalysisEnabled", m_bulkAnalysisEnabled);
    s.endGroup();
    s.beginGroup("Behavior");
    s.setValue("highlightColor", highlightColor.name());
    s.endGroup();
    s.sync();
    return s.status() == QSettings::NoError;
}

QStringList DltChatPlugin::infoConfig()
{
    QStringList info;
    info << QString("Analyzer: %1").arg(m_currentAnalyzerType);
    if (m_analyzer) info << m_analyzer->configurationInfo();
    return info;
}

QWidget* DltChatPlugin::initViewer()
{
    form = new DltChat::Form();
    connect(form, &DltChat::Form::querySubmitted, this, &DltChatPlugin::onQuerySubmitted);
    connect(form, &DltChat::Form::quickActionTriggered, this, &DltChatPlugin::onQuickActionQuery);
    connect(form, &DltChat::Form::nativeFilterTriggered, this, &DltChatPlugin::onNativeFilterTriggered);
    connect(form, &DltChat::Form::aiQuerySubmitted, this, &DltChatPlugin::onAiQuerySubmitted);
    connect(form, &DltChat::Form::configureAiClicked, this, &DltChatPlugin::onConfigureAiClicked);
    connect(form, &DltChat::Form::indexActivated, this, &DltChatPlugin::onIndexActivated);
    connect(form, &DltChat::Form::clearHighlightsRequested, this, &DltChatPlugin::onClearHighlightsRequested);
    connect(form, &DltChat::Form::exportRequested, this, &DltChatPlugin::onExportRequested);
    connect(form, &DltChat::Form::exportAllRequested, this, &DltChatPlugin::onExportAllRequested);
    connect(this, &DltChatPlugin::statusChanged, form, &DltChat::Form::setStatusText, Qt::QueuedConnection);
    connect(this, &DltChatPlugin::onAiAvailabilityChanged, form, &DltChat::Form::setAiStatus, Qt::QueuedConnection);
    connect(form, &DltChat::Form::userFilterLoadRequested,
            this, &DltChatPlugin::onUserFilterLoadRequested);

    form->setDataFetcher([this](int logIndex) {
        QMutexLocker l(&entriesMutex);
        int pos = indexToPos.value(logIndex, -1);
        dltchat::LogEntryData d;
        if (pos >= 0 && pos < entries.size()) {
            const auto &e = entries[pos];
            d.snippet = e.payload.left(120);
            d.level = e.level;
            
            bool dk = form->palette().color(QPalette::Window).lightness() < 128;
            if (e.level == "error" || e.level == "fatal") d.color = QColor(dk ? "#ef5350" : "#d32f2f");
            else if (e.level == "warn") d.color = QColor(dk ? "#ffa726" : "#e65100");
            else d.color = QColor(dk ? "#e0e0e0" : "#424242");
        }
        return d;
    });

    applyConfigToForm();
    if (!m_nativeFilterCatalog.isLoaded())
        loadNativeFilterCatalog();
    populateNativeFilterMenus();
    return form;
}

void DltChatPlugin::applyConfigToForm()
{
    if (!form) return;
    emit onAiAvailabilityChanged(m_aiState, m_aiModelName);
}

void DltChatPlugin::initFileStart(QDltFile *file)
{
    dltFile = file;
    clearData();
    if (form) form->clearQuickActionCache();
    filterRowMapDirty = true;
    if (dltFile)
    {
        int totalMsgs = dltFile->size();
        if (totalMsgs > 0)
            entries.reserve(totalMsgs);
    }
    updateStatus("Loading log file...");
}

void DltChatPlugin::initFileFinish()
{
    rebuildFilterRowMap();
    updateDomainStatus();

    // Kick off the off-main-thread ingestion pipeline so the AI gets a
    // panoramic digest of the whole log without ever blocking the GUI.
    // No spinner / no progress widget: queries asked before ready() use
    // the rule-based fallback that's been in place since day one.
    if (m_ingestionPipeline) {
        QVector<DltAnalyzerInterface::LogEntry> snap;
        {
            QMutexLocker lk(&entriesMutex);
            snap = entries;
        }
        m_hierStore.clear();
        if (!snap.isEmpty())
            m_ingestionPipeline->startAsync(std::move(snap));
    }

    if (m_bulkAnalysisEnabled && m_llmAnalyzer && m_llmAnalyzer->isAvailable())
    {
        startBulkAnalysis();
    }
}

void DltChatPlugin::initMsg(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }
void DltChatPlugin::initMsgDecoded(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }
void DltChatPlugin::updateFileStart() {}
void DltChatPlugin::updateMsg(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }
void DltChatPlugin::updateMsgDecoded(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }

void DltChatPlugin::updateFileFinish()
{
    rebuildFilterRowMap();
    updateDomainStatus();
}

void DltChatPlugin::selectedIdxMsg(int index, QDltMsg &)
{
    if (index >= 0) {
        m_lastSelectedIndices.clear();
        m_lastSelectedIndices.append(index);
    }
}
void DltChatPlugin::selectedIdxMsgDecoded(int index, QDltMsg &)
{
    if (index >= 0) {
        m_lastSelectedIndices.clear();
        m_lastSelectedIndices.append(index);
    }
}

bool DltChatPlugin::initControl(QDltControl *) { return true; }
bool DltChatPlugin::initConnections(QStringList) { return true; }
bool DltChatPlugin::controlMsg(int, QDltMsg &) { return true; }
bool DltChatPlugin::stateChanged(int, QDltConnection::QDltConnectionState, QString) { return true; }
bool DltChatPlugin::autoscrollStateChanged(bool) { return true; }
void DltChatPlugin::initMessageDecoder(QDltMessageDecoder *p) { messageDecoder = p; }
void DltChatPlugin::initMainTableView(QTableView *p)
{
    mainTableView = p;
    // Install the multicolour highlight delegate, chaining the host's original
    // delegate so untouched rows keep their native rendering.
    if (mainTableView && !m_highlightDelegate) {
        m_highlightDelegate = new DltChat::HighlightDelegate(mainTableView);
        m_highlightDelegate->setInnerDelegate(mainTableView->itemDelegate());
        mainTableView->setItemDelegate(m_highlightDelegate);
    }
}
void DltChatPlugin::configurationChanged() {}

void DltChatPlugin::setAnalyzerType(const QString &type)
{
    if (type == "llm" && m_llmAnalyzer && m_aiState == 2)
    {
        m_analyzer = m_llmAnalyzer;
        m_currentAnalyzerType = "llm";
    }
    else
    {
        m_analyzer = m_ruleBasedAnalyzer;
        m_currentAnalyzerType = "rule-based";
    }
}

QString DltChatPlugin::currentAnalyzerType() const { return m_currentAnalyzerType; }

void DltChatPlugin::configureLlmAnalyzer(const QString &ep, const QString &key, const QString &model)
{
    if (!m_llmAnalyzer) m_llmAnalyzer = new DltLlmAnalyzerInterface(this);
    m_llmAnalyzer->setApiEndpoint(ep);
    m_llmAnalyzer->setApiKey(key);
    m_llmAnalyzer->setModelName(model);
    m_aiAvailabilityRetryCount = 0;
    m_aiResponseCache.clear();
    checkAiAvailabilityAsync();
    applyConfigToForm();
}

QStringList DltChatPlugin::extractKeywords(const QString &text) const
{
    QStringList tokens = text.toLower().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    QStringList kw;
    kw.reserve(tokens.size());
    for (const QString &t : tokens)
    {
        if (t.size() < 3) continue;
        if (indexStopwords.contains(t)) continue;
        if (t.at(0).isDigit()) continue;
        kw.append(t);
    }
    kw.removeDuplicates();
    return kw;
}

QString DltChatPlugin::buildUserFilterContextHtml() const
{
    if (!m_userFilterManager || m_userFilterManager->activeFilterCount() == 0)
        return QString();

    QString html = "<br><small><b>Filtri attivi:</b> ";
    QStringList names;
    for (const auto &f : m_userFilterManager->filters()) {
        if (f.enabled && f.isValid)
            names.append(f.label);
    }
    html += names.join(", ");
    html += "</small>";
    return html;
}

void DltChatPlugin::onQuickActionQuery(const QString &query)
{
    if (!form) return;

    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }

    if (snapshot.isEmpty()) {
        form->appendMessage("Tu", query.toHtmlEscaped());
        form->appendMessage("Chat Assistant", "Nessun log caricato. Apri un file DLT.");
        form->setResults(QList<int>());
        return;
    }

    // Check cache first
    QList<int> cached = form->cachedQuickActionResult(query);
    if (!cached.isEmpty()) {
        form->appendMessage("Tu", query.toHtmlEscaped());
        if (cached.isEmpty()) {
            form->appendMessage("Chat Assistant",
                QString("Nessun messaggio per <b>%1</b> (cache).").arg(query.toHtmlEscaped()));
        } else {
            form->appendMessage("Chat Assistant",
                QString("Trovati <b>%1</b> risultati per <b>%2</b> (cache).")
                .arg(cached.size()).arg(query.toHtmlEscaped()));
        }
        form->setResults(cached);
        highlightIndices(cached);
        return;
    }

    QElapsedTimer timer;
    timer.start();

    const auto &registry = dltchat::CategoryRegistry::instance();
    const auto resolved = registry.resolveQuery(query.trimmed().toLower());
    QList<int> indices;

    if (resolved.kind == dltchat::ResolvedQuery::Kind::Category
        || resolved.kind == dltchat::ResolvedQuery::Kind::CategoryById
        || resolved.kind == dltchat::ResolvedQuery::Kind::CombinedFilter
        || resolved.kind == dltchat::ResolvedQuery::Kind::ProjectionEvent) {
        auto filtered = registry.filterEntries(snapshot, resolved);
        QString filterLabel = resolved.categoryId.isEmpty() ? query : resolved.categoryId;
        if (resolved.kind == dltchat::ResolvedQuery::Kind::ProjectionEvent)
            filterLabel = resolved.projectionEventId;
        if (resolved.kind == dltchat::ResolvedQuery::Kind::CombinedFilter)
            filterLabel = resolved.combinedId;

        form->appendMessage("Tu", query.toHtmlEscaped());
        if (filtered.isEmpty()) {
            form->appendMessage("Chat Assistant",
                QString("Nessun messaggio <b>%1</b> trovato.").arg(filterLabel));
        } else {
            int totalLevels[6] = {0};
            for (const auto &e : filtered) {
                indices.append(e.index);
                if (e.level == "fatal") totalLevels[0]++;
                else if (e.level == "error") totalLevels[1]++;
                else if (e.level == "warn") totalLevels[2]++;
                else if (e.level == "info") totalLevels[3]++;
                else if (e.level == "debug") totalLevels[4]++;
                else totalLevels[5]++;
            }
            QString html = QString("Trovati <b>%1</b> messaggi per <b>%2</b> su %3 totali.")
                .arg(filtered.size()).arg(filterLabel).arg(snapshot.size());
            html += QString("<br><small>");
            QStringList parts;
            if (totalLevels[0]) parts += QString("fatal:%1").arg(totalLevels[0]);
            if (totalLevels[1]) parts += QString("error:%1").arg(totalLevels[1]);
            if (totalLevels[2]) parts += QString("warn:%1").arg(totalLevels[2]);
            if (totalLevels[3]) parts += QString("info:%1").arg(totalLevels[3]);
            if (totalLevels[4]) parts += QString("debug:%1").arg(totalLevels[4]);
            if (totalLevels[5]) parts += QString("other:%1").arg(totalLevels[5]);
            html += parts.join(", ");
            html += QString(" | %1ms</small>").arg(timer.elapsed());
            form->appendMessage("Chat Assistant", html);
        }
        form->storeQuickActionResult(query, indices);
        form->setResults(indices);
        highlightIndices(indices);
    } else {
        // Fallback: normal routing handles Tu echo
        onQuerySubmitted(query);
    }
}

void DltChatPlugin::onQuerySubmitted(const QString &query)
{
    if (!form) return;
    form->appendMessage("Tu", query.toHtmlEscaped());

    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }

    if (snapshot.isEmpty()) {
        form->appendMessage("Chat Assistant", "Nessun log caricato. Apri un file DLT.");
        form->setResults(QList<int>());
        return;
    }

    QElapsedTimer timer;
    timer.start();

    QString lq = query.trimmed().toLower();

    // 0. Bulk: tag: / category: query routing
    if (lq.startsWith("tag:") && m_bulkAnalyzer && m_bulkAnalyzer->hasCompleted()) {
        QString tag = lq.mid(4).trimmed();
        QList<int> indices;
        m_bulkAnalyzer->searchByTag(tag, indices);
        if (indices.isEmpty()) {
            form->appendMessage("Chat Assistant",
                QString("Nessun risultato per tag <b>%1</b>.").arg(tag.toHtmlEscaped()));
        } else {
            QSet<QString> cats;
            for (int idx : indices)
                cats.insert(m_bulkAnalyzer->getCategoryForIndex(idx));
            cats.remove("unknown");
            QString html = QString("Trovati <b>%1</b> risultati per tag <b>%2</b>.<br>")
                .arg(indices.size()).arg(tag.toHtmlEscaped());
            if (!cats.isEmpty() && cats.size() <= 15)
                html += QString("Categorie: %1").arg(QStringList(cats.values()).join(", "));
            form->appendMessage("Chat Assistant", html);
        }
        form->setResults(indices);
        highlightIndices(indices);
        return;
    }
    if (lq.startsWith("category:") && m_bulkAnalyzer && m_bulkAnalyzer->hasCompleted()) {
        QString cat = lq.mid(9).trimmed();
        QList<int> indices;
        m_bulkAnalyzer->searchByCategory(cat, indices);
        if (indices.isEmpty()) {
            form->appendMessage("Chat Assistant",
                QString("Nessun risultato per categoria <b>%1</b>.").arg(cat.toHtmlEscaped()));
        } else {
            QString html = QString("Trovati <b>%1</b> risultati per categoria <b>%2</b>.<br>")
                .arg(indices.size()).arg(cat.toHtmlEscaped());
            form->appendMessage("Chat Assistant", html);
        }
        form->setResults(indices);
        highlightIndices(indices);
        return;
    }

    // 1. Registry-driven category / projection / combined filters
    const auto &registry = dltchat::CategoryRegistry::instance();
    const auto resolved = registry.resolveQuery(lq);
    if (resolved.kind == dltchat::ResolvedQuery::Kind::Category
        || resolved.kind == dltchat::ResolvedQuery::Kind::CategoryById
        || resolved.kind == dltchat::ResolvedQuery::Kind::CombinedFilter
        || resolved.kind == dltchat::ResolvedQuery::Kind::ProjectionEvent) {
        auto filtered = registry.filterEntries(snapshot, resolved);
        DltAnalyzerInterface::QueryResult result;
        QString filterLabel = resolved.categoryId.isEmpty() ? lq : resolved.categoryId;
        if (resolved.kind == dltchat::ResolvedQuery::Kind::ProjectionEvent)
            filterLabel = resolved.projectionEventId;
        if (resolved.kind == dltchat::ResolvedQuery::Kind::CombinedFilter)
            filterLabel = resolved.combinedId;
        if (filtered.isEmpty()) {
            result.responseHtml = QString("Nessun messaggio <b>%1</b> trovato.").arg(filterLabel);
            result.success = true;
        } else {
            int totalLevels[6] = {0};
            for (const auto &e : filtered) {
                result.indices.append(e.index);
                result.snippets.append(e.payload.left(120));
                if (e.level == "fatal") totalLevels[0]++;
                else if (e.level == "error") totalLevels[1]++;
                else if (e.level == "warn") totalLevels[2]++;
                else if (e.level == "info") totalLevels[3]++;
                else if (e.level == "debug") totalLevels[4]++;
                else totalLevels[5]++;
            }
            QString breakdown;
            QStringList parts;
            if (totalLevels[0]) parts += QString("fatal:%1").arg(totalLevels[0]);
            if (totalLevels[1]) parts += QString("error:%1").arg(totalLevels[1]);
            if (totalLevels[2]) parts += QString("warn:%1").arg(totalLevels[2]);
            if (totalLevels[3]) parts += QString("info:%1").arg(totalLevels[3]);
            if (totalLevels[4]) parts += QString("debug:%1").arg(totalLevels[4]);
            if (totalLevels[5]) parts += QString("other:%1").arg(totalLevels[5]);
            breakdown = parts.join(", ");
            result.responseHtml = QString("Trovati <b>%1</b> messaggi per <b>%2</b> su %3 totali.<br><small>%4</small>")
                .arg(filtered.size()).arg(filterLabel).arg(snapshot.size()).arg(breakdown);
            result.success = true;
        }
        result.processingTimeMs = timer.elapsed();
        QString html = result.responseHtml;
        html += buildUserFilterContextHtml();
        html += QString("<br><small>%1ms</small>").arg(result.processingTimeMs);
        form->appendMessage("Chat Assistant", html);
            form->setResults(result.indices);
        highlightIndices(result.indices);
        return;
    }

    // 2. Special commands for rule-based analyzer
    bool isSpecial = registry.isSpecialCommand(lq);
    if (!isSpecial) {
        if (lq.contains("error") || lq.contains("errore") || lq.contains("fatal")
            || lq.contains("warn") || lq == "info" || lq == "debug" || lq == "verbose")
            isSpecial = true;
    }
    if (!isSpecial && resolved.kind == dltchat::ResolvedQuery::Kind::SpecialCommand)
        isSpecial = true;

    if (!isSpecial) {
        QList<int> searchIndices = liveSearch(lq);

        form->setResults(searchIndices);
        highlightIndices(searchIndices);
        return;
    }

    // 3. Special commands delegate to rule-based analyzer
    {
        DltAnalyzerInterface::QueryResult result = m_ruleBasedAnalyzer->analyzeQuery(lq, snapshot);
        result.processingTimeMs = timer.elapsed();
        QString html = result.responseHtml;
        html += buildUserFilterContextHtml();
        html += QString("<br><small>%1ms</small>").arg(result.processingTimeMs);
        form->appendMessage("Chat Assistant", html);
        form->setResults(result.indices);
        highlightIndices(result.indices);
    }
}

void DltChatPlugin::onAiQuerySubmitted(const QString &query)
{
    if (!form) return;

    {
        QMutexLocker lk(&m_llmMutex);
        if (m_llmRequestInProgress) {
            qint64 elapsed = m_llmRequestTimer.elapsed();
            if (elapsed < kAIDebounceMs) {
                form->appendMessage("AI Assistant",
                    "Richiesta gia in elaborazione. Attendere...");
                return;
            }
        }
    }

    form->appendMessage("Tu (AI)", query.toHtmlEscaped());
    form->setProcessingProgress(true);

    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }

    if (snapshot.isEmpty()) {
        form->setProcessingProgress(false);
        form->appendMessage("AI Assistant", "Nessun log caricato. Apri un file DLT.");
        return;
    }

    if (!m_llmAnalyzer)
    {
        form->setProcessingProgress(false);
        auto result = m_ruleBasedAnalyzer->analyzeQuery(query, snapshot);
        QString html = result.responseHtml + "<br><em>AI non disponibile, analisi locale.</em>";
        html += buildUserFilterContextHtml();
        form->appendMessage("AI Assistant (fallback)", html);
        form->setResults(result.indices);
        highlightIndices(result.indices);
        return;
    }

    // Provider/model detection drives the budget plan and map-reduce
    // concurrency. We resolve them once up-front and reuse for both paths.
    const QString provider = m_llmAnalyzer->detectProviderType();
    const QString model    = m_llmAnalyzer->modelName();

    QString filterCtx;
    if (m_userFilterManager && m_userFilterManager->activeFilterCount() > 0) {
        QStringList parts;
        for (const auto &f : m_userFilterManager->filters()) {
            if (f.enabled && f.isValid)
                parts.append(QStringLiteral("%1 (pattern: %2)").arg(f.label, f.regex.pattern()));
        }
        if (!parts.isEmpty())
            filterCtx = QStringLiteral("User-defined active filters:\n") + parts.join(QLatin1Char('\n'));
    }

    if (!m_llmAnalyzer->conversationManager())
        m_llmAnalyzer->setConversationManager(&m_conversationManager);

    // Route global queries (or explicit `deep:` prefix) through map-reduce
    // when the ingestion pipeline has produced enough block summaries to
    // make sharding meaningful; otherwise fall through to single-shot.
    const bool wantDeep = ContextBudgetPlanner::hasDeepPrefix(query);
    const bool isGlobal = wantDeep || ContextBudgetPlanner::isGlobalQuery(query);
    const auto blocks = m_hierStore.blocks();

    if (isGlobal && blocks.size() >= 2 && m_hierStore.isReady()) {
        if (m_mapReduceAnalyzer->isRunning()) {
            form->setProcessingProgress(false);
            form->appendMessage("AI Assistant", "Map-reduce gia in corso. Attendere il completamento.");
            return;
        }
        {
            QMutexLocker lk(&m_llmMutex);
            m_llmRequestInProgress = true;
            m_llmRequestTimer.start();
        }
        m_pendingAiQuery = query;
        m_pendingAiIsMapReduce = true;

        MapReduceAnalyzer::Config cfg;
        cfg.provider = provider;
        cfg.model = model;
        cfg.maxShards = 32;

        // The hierarchical digest is still useful as map-step context: each
        // shard sees its block + the global overview so it can place its
        // observations in the broader picture.
        m_llmAnalyzer->setExtraContext(filterCtx);
        m_llmAnalyzer->setHierarchicalDigest(
            m_hierStore.compactDigest(8000));
        if (m_fibexEnricher.isLoaded())
            m_llmAnalyzer->setFibexLoaded(true);
        else
            m_llmAnalyzer->setFibexLoaded(false);

        if (!m_mapReduceAnalyzer->runAsync(query, snapshot, blocks, cfg)) {
            QMutexLocker lk(&m_llmMutex);
            m_llmRequestInProgress = false;
            m_pendingAiIsMapReduce = false;
        }
        return;
    }

    // Single-shot path: delegate retrieval/enrich/correlate/digest/cache-key
    // assembly to the off-main-thread AiQueryPipeline. The GUI used to do
    // all of this inline and freeze for seconds on million-row logs.
    if (m_aiQueryPipeline->isBusy()) {
        form->setProcessingProgress(false);
        form->appendMessage("AI Assistant", "Preparazione query AI gia in corso. Attendere.");
        return;
    }
    m_pendingAiQuery = query;
    m_pendingAiIsMapReduce = false;

    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = true;
        m_llmRequestTimer.start();
    }

    dltchat::AiQueryPipeline::Request req;
    req.query    = query;
    req.provider = provider;
    req.model    = model;
    req.snapshot = std::move(snapshot);
    {
        QMutexLocker lk(&entriesMutex);
        req.invertedIndex = invertedIndex;
    }
    req.selectedIndices = m_lastSelectedIndices;
    req.fibex = &m_fibexEnricher;
    req.hierStore = &m_hierStore;
    req.userFilterContext = filterCtx;
    req.conversationHistoryChars = 0;
    m_aiQueryPipeline->executeAsync(req);
}

void DltChatPlugin::onLlmResultReady(const DltAnalyzerInterface::QueryResult &result, const QString &originalQuery)
{
    if (!form) return;
    form->setProcessingProgress(false);

    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = false;
    }

    if (result.success) {
        QMutexLocker lk(&m_aiCacheMutex);
        QString cacheKey = buildAiCacheKey(originalQuery, m_lastAiContext);
        if (m_aiResponseCache.size() >= kAICacheMaxEntries)
            m_aiResponseCache.clear();
        m_aiResponseCache.insert(cacheKey, result);
    }

    QString html = result.responseHtml;
    if (!result.success)
    {
        QVector<DltAnalyzerInterface::LogEntry> snapshot;
        { QMutexLocker l(&entriesMutex); snapshot = entries; }
        auto fb = m_ruleBasedAnalyzer->analyzeQuery(originalQuery, snapshot);
        html = fb.responseHtml + "<br><em>AI non disponibile. Risultato locale.</em>";
        if (fb.processingTimeMs > 0)
            html += QString("<br><small>%1ms</small>").arg(fb.processingTimeMs);
        html += buildUserFilterContextHtml();
        form->appendMessage("AI Assistant", html);
        form->setResults(fb.indices);
        highlightIndices(fb.indices);
        return;
    }

    html += buildUserFilterContextHtml();
    form->appendMessage("AI Assistant", html);
    form->setResults(result.indices);
    highlightIndices(result.indices);
}

void DltChatPlugin::onConfigureAiClicked()
{
    if (!m_llmAnalyzer || !form) return;
    DltAiOptionsDialog dlg(form);
    dlg.setWindowTitle(tr("AI Configuration"));
    dlg.setEndpoint(m_llmAnalyzer->apiEndpoint());
    dlg.setApiKey(m_llmAnalyzer->apiKey());
    dlg.setModel(m_llmAnalyzer->modelName());
    dlg.setMaxTokens(m_llmAnalyzer->maxTokens());
    dlg.setTemperature(m_llmAnalyzer->temperature());
    dlg.setTimeoutMs(m_llmAnalyzer->timeout());
    if (!m_copilotOAuthToken.isEmpty())
        dlg.setCopilotOAuthToken(m_copilotOAuthToken);
    if (!m_copilotClientId.isEmpty())
        dlg.setOauthClientId(m_copilotClientId);

    connect(&dlg, &DltAiOptionsDialog::copilotTokenObtained, this, [this](const QString &tok) {
        m_copilotOAuthToken = tok;
        m_llmAnalyzer->setApiKey(tok);
        m_aiResponseCache.clear();
        checkAiAvailabilityAsync();
    });

    if (dlg.exec() == QDialog::Accepted)
    {
        m_llmAnalyzer->setApiEndpoint(dlg.endpoint());
        // Use Copilot token as the API key when Copilot is configured
        QString key = dlg.apiKey();
        if (key.isEmpty() && !dlg.copilotOAuthToken().isEmpty())
            key = dlg.copilotOAuthToken();
        m_llmAnalyzer->setApiKey(key);
        m_llmAnalyzer->setModelName(dlg.model());
        m_llmAnalyzer->setMaxTokens(dlg.maxTokens());
        m_llmAnalyzer->setTemperature(dlg.temperature());
        m_llmAnalyzer->setTimeout(dlg.timeoutMs());
        if (!dlg.copilotOAuthToken().isEmpty())
            m_copilotOAuthToken = dlg.copilotOAuthToken();
        m_copilotClientId = dlg.oauthClientId();
        m_aiAvailabilityRetryCount = 0;
        m_aiResponseCache.clear();
        checkAiAvailabilityAsync();
    }
}

void DltChatPlugin::onIndexActivated(int index)
{
    if (!mainTableView || !dltFile)
    {
        if (form) form->appendMessage("Chat Assistant", "Navigazione non disponibile.");
        return;
    }
    int row = findRowForIndex(index);
    if (row < 0)
    {
        if (form) form->appendMessage("Chat Assistant", "Indice non trovato (filtri?).");
        return;
    }
    QModelIndex ti = mainTableView->model()->index(row, 0);
    if (!ti.isValid()) return;
    if (mainTableView->selectionModel())
        mainTableView->selectionModel()->setCurrentIndex(ti, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    mainTableView->scrollTo(ti, QAbstractItemView::PositionAtCenter);
}

void DltChatPlugin::onClearHighlightsRequested()
{
    if (m_highlightDelegate) {
        m_highlightDelegate->clear();
        if (mainTableView && mainTableView->viewport())
            mainTableView->viewport()->update();
    }
    highlightIndices(QList<int>());
    if (form) form->setResults(QList<int>());
}

void DltChatPlugin::onExportRequested(const QString &filePath, const QList<int> &indices,
                                       const QStringList &snippets, const QString &query)
{
    if (!form) return;
    if (indices.isEmpty()) {
        form->appendMessage("Chat Assistant", "Nessun indice da esportare.");
        return;
    }
    QVector<DltAnalyzerInterface::LogEntry> snap;
    { QMutexLocker l(&entriesMutex); snap = entries; }
    if (snap.isEmpty()) {
        form->appendMessage("Chat Assistant", "Nessun log caricato.");
        return;
    }
    bool ok = DltExport::exportToCsv(filePath, indices, snippets, query, snap);
    form->appendMessage("Chat Assistant",
        ok ? QString("Esportati: %1").arg(filePath) : "Errore esportazione.");
}

void DltChatPlugin::onExportAllRequested(const QString &filePath)
{
    if (!form) return;
    QVector<DltAnalyzerInterface::LogEntry> snap;
    { QMutexLocker l(&entriesMutex); snap = entries; }
    if (snap.isEmpty()) {
        form->appendMessage("Chat Assistant", "Nessun log caricato.");
        return;
    }
    bool ok = DltExport::exportAllEntries(filePath, snap);
    form->appendMessage("Chat Assistant",
        ok ? QString("Tutti esportati: %1 (%2)").arg(filePath).arg(snap.size()) : "Errore esportazione.");
}

void DltChatPlugin::clearData()
{
    if (m_ingestionPipeline)
        m_ingestionPipeline->cancel();
    if (m_aiQueryPipeline)
        m_aiQueryPipeline->cancel();
    if (m_mapReduceAnalyzer)
        m_mapReduceAnalyzer->cancel();
    m_hierStore.clear();

    QMutexLocker l(&entriesMutex);
    entries.clear();
    entries.squeeze();
    indexToPos.clear();
    invertedIndex.clear();
    m_aiResponseCache.clear();
}

void DltChatPlugin::ingestMessage(int index, QDltMsg &msg)
{
    if (!dltFile) return;
    QMutexLocker l(&entriesMutex);

    if (indexToPos.contains(index)) return;

    QDltMsg m = msg;
    if (messageDecoder) messageDecoder->decodeMsg(m, 0);

    DltAnalyzerInterface::LogEntry e;
    e.index = index;
    e.time = QString("%1.%2").arg(m.getTimeString()).arg(m.getMicroseconds(), 6, 10, QLatin1Char('0'));
    e.timestamp = QString("%1.%2").arg(m.getTimestamp() / 10000).arg(m.getTimestamp() % 10000, 4, 10, QLatin1Char('0'));
    e.ecu = m.getEcuid();
    e.apid = m.getApid();
    e.ctid = m.getCtid();
    e.level = m.getSubtypeString().toLower();
    e.payload = DltRuleBasedAnalyzer::simplifyPayload(m.toStringPayload());

    AutomotiveLogParser::classify(e);

    indexToPos.insert(index, entries.size());
    entries.append(e);

    QStringList kw = extractKeywords(e.payload);
    kw.append(e.level);
    kw.append(e.apid.toLower());
    kw.append(e.ctid.toLower());
    kw.append(e.domain.toLower());
    kw.removeDuplicates();
    if (!e.event.isEmpty())
        invertedIndex[QString("event:") + e.event].insert(index);
    invertedIndex[QString("domain:") + e.domain.toLower()].insert(index);
    if (!e.category.isEmpty())
        invertedIndex[QString("category:") + e.category.toLower()].insert(index);
    for (const QString &k : kw)
        invertedIndex[k].insert(index);
}

void DltChatPlugin::rebuildFilterRowMap() const
{
    if (!dltFile) return;
    filterRowMap.clear();
    int rows = dltFile->sizeFilter();
    filterRowMap.reserve(rows);
    for (int r = 0; r < rows; ++r)
        filterRowMap.insert(dltFile->getMsgFilterPos(r), r);
    filterRowMapDirty = false;
}

int DltChatPlugin::findRowForIndex(int index) const
{
    if (!dltFile) return -1;
    if (filterRowMapDirty)
        rebuildFilterRowMap();
    return filterRowMap.value(index, -1);
}

void DltChatPlugin::highlightIndices(const QList<int> &indices)
{
    if (!dltFile) return;
    // Drop any per-filter colours from a previous native .dlp action so they do
    // not bleed into a subsequent monochrome (free-text / level) highlight.
    if (m_highlightDelegate && !m_highlightDelegate->isEmpty())
        m_highlightDelegate->clear();
    QSet<unsigned long int> seen;
    seen.reserve(indices.size() + m_highlightMap.size());
    QList<unsigned long int> m;
    m.reserve(indices.size() + m_highlightMap.size());
    for (int i : indices) {
        if (i >= 0) {
            unsigned long ul = static_cast<unsigned long>(i);
            if (!seen.contains(ul)) {
                seen.insert(ul);
                m.append(ul);
            }
        }
    }
    for (auto it = m_highlightMap.constBegin(); it != m_highlightMap.constEnd(); ++it) {
        if (it.key() >= 0) {
            unsigned long ul = static_cast<unsigned long>(it.key());
            if (!seen.contains(ul)) {
                seen.insert(ul);
                m.append(ul);
            }
        }
    }
#ifdef QDLT_HAS_SET_MANUAL_MARKER_INDICES
    dltFile->setManualMarkerIndices(m);
#else
    // DLT Viewer < 2.30 (e.g. ARTIST8 2.28) has no setManualMarkerIndices:
    // highlight matched messages by selecting their rows in the host main
    // table instead. An empty list clears the previous selection.
    if (mainTableView && mainTableView->model() && mainTableView->selectionModel())
    {
        QAbstractItemModel *model = mainTableView->model();
        const int lastCol = model->columnCount() - 1;
        QItemSelection selection;
        for (unsigned long int ul : m)
        {
            int row = findRowForIndex(static_cast<int>(ul));
            if (row < 0) continue;
            const QModelIndex topLeft = model->index(row, 0);
            const QModelIndex bottomRight = model->index(row, lastCol);
            if (topLeft.isValid() && bottomRight.isValid())
                selection.select(topLeft, bottomRight);
        }
        mainTableView->selectionModel()->select(
            selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
#endif
    if (mainTableView) mainTableView->viewport()->update();
}

void DltChatPlugin::loadNativeFilterCatalog()
{
    if (!m_nativeFilterCatalog.load(m_dlpFilterPath)) {
        updateStatus(QStringLiteral("Filtri .dlp non caricati: %1")
                         .arg(m_nativeFilterCatalog.lastError()));
    } else {
        updateStatus(QStringLiteral("Filtri .dlp caricati da %1")
                         .arg(m_nativeFilterCatalog.sourcePath()));
    }
}

void DltChatPlugin::populateNativeFilterMenus()
{
    if (!form) return;
    QVector<DltChat::Form::NativeMenuSpec> specs;
    for (const DltChat::NativeFilterGroup &g : m_nativeFilterCatalog.groups()) {
        DltChat::Form::NativeMenuSpec spec;
        spec.label = g.label;
        for (const QString &name : g.actionNames) {
            const DltChat::NativeFilterAction *a = m_nativeFilterCatalog.action(name);
            spec.actions.append(qMakePair(name, a ? a->colour : QColor()));
        }
        if (!spec.actions.isEmpty())
            specs.append(spec);
    }
    form->buildNativeFilterMenus(specs);
}

void DltChatPlugin::onNativeFilterTriggered(const QString &filterName)
{
    if (!form) return;
    form->appendMessage("Tu", filterName.toHtmlEscaped());

    const DltChat::NativeFilterAction *action = m_nativeFilterCatalog.action(filterName);
    if (!action) {
        form->appendMessage("Chat Assistant",
                            tr("Filtro '%1' non trovato nel catalogo .dlp.").arg(filterName.toHtmlEscaped()));
        return;
    }
    if (!dltFile) {
        form->appendMessage("Chat Assistant", tr("Nessun file DLT aperto."));
        return;
    }

    // Cache key namespaced to avoid clashing with free-text quick-action queries.
    const QString cacheKey = QStringLiteral("dlp:") + filterName;
    QList<int> indices = form->cachedQuickActionResult(cacheKey);
    if (indices.isEmpty()) {
        indices = m_nativeFilterCatalog.match(*action, dltFile);
        form->storeQuickActionResult(cacheKey, indices);
    }

    const QString colourName = action->colour.isValid() ? action->colour.name() : QStringLiteral("#888888");
    const QString summary = tr("Filtro nativo <b>%1</b> — App=%2 Ctx=%3%4 — "
                               "<span style='background:%5'>&nbsp;&nbsp;&nbsp;</span> "
                               "%6 righe corrispondenti.")
        .arg(filterName.toHtmlEscaped(),
             action->positive.apid.toHtmlEscaped(),
             action->positive.ctid.toHtmlEscaped(),
             action->hasExcludes() ? tr(" (con esclusioni)") : QString(),
             colourName)
        .arg(indices.size());
    form->appendMessage("Chat Assistant", summary);

    form->setResults(indices);
    highlightIndicesColored(indices, action->colour);
}

void DltChatPlugin::highlightIndicesColored(const QList<int> &indices, const QColor &color)
{
    m_lastSelectedIndices = indices;
    if (!m_highlightDelegate || !mainTableView) {
        // No delegate available (e.g. table view not yet wired): fall back to the
        // monochrome marker/selection path so highlighting still happens.
        highlightIndices(indices);
        return;
    }

    const QColor c = color.isValid() ? color : highlightColor;
    QHash<int, QColor> rowColors;
    rowColors.reserve(indices.size());
    for (int idx : indices) {
        const int row = findRowForIndex(idx);
        if (row >= 0)
            rowColors.insert(row, c);
    }
    m_highlightDelegate->setRowColors(rowColors);
    if (mainTableView->viewport())
        mainTableView->viewport()->update();
}

void DltChatPlugin::updateStatus(const QString &text)
{
    emit statusChanged(text);
}

QList<int> DltChatPlugin::liveSearch(const QString &rawQuery) const
{
    if (rawQuery.isEmpty()) return {};

    QStringList kw = extractKeywords(rawQuery);
    QMutexLocker lk(&entriesMutex);

    if (kw.isEmpty()) {
        QList<int> results;
        
        // Parallel filtering to gather matching entries
        auto filterFn = [rawQuery](const dltchat::DltAnalyzerInterface::LogEntry &e) {
            return e.payload.contains(rawQuery, Qt::CaseInsensitive) ||
                   e.apid.contains(rawQuery, Qt::CaseInsensitive) ||
                   e.ctid.contains(rawQuery, Qt::CaseInsensitive) ||
                   e.ecu.contains(rawQuery, Qt::CaseInsensitive) ||
                   e.level.contains(rawQuery, Qt::CaseInsensitive) ||
                   e.domain.contains(rawQuery, Qt::CaseInsensitive);
        };
        
        QVector<dltchat::DltAnalyzerInterface::LogEntry> matchedEntries = QtConcurrent::blockingFiltered(entries, filterFn);
        
        results.reserve(matchedEntries.size());
        for (const auto &e : matchedEntries) {
            results.append(e.index);
        }
        std::sort(results.begin(), results.end());
        return results;
    }

    QSet<int> candidates;
    bool first = true;
    for (const QString &k : kw) {
        QSet<int> set = invertedIndex.value(k);
        if (set.isEmpty()) return {};
        if (first) {
            candidates = set;
            first = false;
        } else {
            candidates.intersect(set);
        }
        if (candidates.isEmpty()) return {};
    }

    QList<int> results;
    
    auto filterFn = [candidates, rawQuery](const dltchat::DltAnalyzerInterface::LogEntry &e) {
        if (!candidates.contains(e.index)) return false;
        return e.payload.contains(rawQuery, Qt::CaseInsensitive) ||
               e.apid.contains(rawQuery, Qt::CaseInsensitive) ||
               e.ctid.contains(rawQuery, Qt::CaseInsensitive) ||
               e.ecu.contains(rawQuery, Qt::CaseInsensitive) ||
               e.level.contains(rawQuery, Qt::CaseInsensitive) ||
               e.domain.contains(rawQuery, Qt::CaseInsensitive) ||
               e.time.contains(rawQuery, Qt::CaseInsensitive);
    };

    QVector<dltchat::DltAnalyzerInterface::LogEntry> matchedEntries = QtConcurrent::blockingFiltered(entries, filterFn);
    
    results.reserve(matchedEntries.size());
    for (const auto &e : matchedEntries) {
        results.append(e.index);
    }
    
    std::sort(results.begin(), results.end());
    return results;
}

void DltChatPlugin::applyUserFilterHighlights()
{
    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }
    m_highlightMap = m_userFilterManager->applyToEntries(snapshot);
}

void DltChatPlugin::updateDomainStatus()
{
    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    int total = 0;
    { QMutexLocker l(&entriesMutex);
      snapshot = entries;
      total = entries.size();
    }

    auto stats = AutomotiveLogParser::domainStats(snapshot);
    int active = m_userFilterManager->activeFilterCount();

    QString text = QString("Loaded %1 msgs | CarPlay: %2 | AA: %3")
        .arg(total).arg(stats.first).arg(stats.second);
    if (active > 0)
        text += QString(" | Filters: %1 active").arg(active);

    QString aiStatus;
    if (m_aiState == 2) aiStatus = " | AI: online";
    else if (m_aiState == 1) aiStatus = " | AI: offline";
    text += aiStatus;

    updateStatus(text);
}

void DltChatPlugin::onUserFilterLoadRequested(const QString &path)
{
    QString error;
    if (m_userFilterManager->loadFromFile(path, &error)) {
        int n = m_userFilterManager->activeFilterCount();
        if (form) form->appendMessage("Chat Assistant",
            QString("Filtri caricati: %1 regole attive da %2.").arg(n).arg(path));
    } else {
        if (form) form->appendMessage("Chat Assistant",
            QString("Errore caricamento filtri: %1").arg(error));
    }
}

// ---------------------------------------------------------------------------
//   AI pipeline slots (off-main-thread retrieval/digest/cache and map-reduce)
// ---------------------------------------------------------------------------

void DltChatPlugin::onAiPipelinePrepared(const dltchat::AiQueryPipeline::Result &prep)
{
    if (!form || !m_llmAnalyzer) return;

    // Cache lookup runs on the GUI thread; the prep step computed a stable
    // SHA1 over query + first 20 entry indices + digest hash.
    {
        QMutexLocker lk(&m_aiCacheMutex);
        auto it = m_aiResponseCache.find(prep.cacheKey);
        if (it != m_aiResponseCache.end()) {
            {
                QMutexLocker llk(&m_llmMutex);
                m_llmRequestInProgress = false;
            }
            form->setProcessingProgress(false);
            const auto &cached = it.value();
            QString html = cached.responseHtml + "<br><small>(risposta cache)</small>";
            html += buildUserFilterContextHtml();
            form->appendMessage("AI Assistant", html);
            form->setResults(cached.indices);
            highlightIndices(cached.indices);
            return;
        }
    }

    if (m_fibexEnricher.isLoaded())
        m_llmAnalyzer->setFibexLoaded(true);
    else
        m_llmAnalyzer->setFibexLoaded(false);

    m_llmAnalyzer->setExtraContext(prep.extraContext);
    m_llmAnalyzer->setHierarchicalDigest(prep.hierarchicalDigest);

    m_lastAiContext = prep.contextEntries;
    if (!m_llmAnalyzer->analyzeQueryAsync(m_pendingAiQuery, prep.contextEntries)) {
        {
            QMutexLocker lk(&m_llmMutex);
            m_llmRequestInProgress = false;
        }
        form->setProcessingProgress(false);
        AiErrorReporter::Context ctx;
        ctx.provider = prep.budget.profile.provider;
        ctx.model    = prep.budget.profile.model;
        ctx.budgetChars = prep.budget.rawChars + prep.budget.hierChars;
        ctx.entryCount  = prep.contextEntries.size();
        ctx.diagnostic  = prep.diagnostic;
        form->appendMessage("AI Assistant",
            AiErrorReporter::formatHtml(
                QStringLiteral("AI/Dispatch"),
                QStringLiteral("analyzeQueryAsync rejected (likely in-progress or unavailable)"),
                ctx));
    }
}

void DltChatPlugin::onAiPipelineFailed(const QString &stage, const QString &reason)
{
    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = false;
    }
    if (!form) return;
    form->setProcessingProgress(false);

    AiErrorReporter::Context ctx;
    if (m_llmAnalyzer) {
        ctx.provider = m_llmAnalyzer->detectProviderType();
        ctx.model    = m_llmAnalyzer->modelName();
    }
    {
        QMutexLocker lk(&entriesMutex);
        ctx.entryCount = entries.size();
    }
    form->appendMessage("AI Assistant",
        AiErrorReporter::formatHtml(stage, reason, ctx));
}

void DltChatPlugin::onMapReduceShardCompleted(int shardIdx, int total, qint64 elapsedMs)
{
    if (!form) return;
    updateStatus(QString("AI map-reduce: shard %1/%2 ok (%3 ms)")
                     .arg(shardIdx + 1).arg(total).arg(elapsedMs));
}

void DltChatPlugin::onMapReduceReduceReady(const dltchat::DltAnalyzerInterface::QueryResult &result,
                                           const QString &originalQuery)
{
    Q_UNUSED(originalQuery);
    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = false;
        m_pendingAiIsMapReduce = false;
    }
    if (!form) return;
    form->setProcessingProgress(false);
    QString html = result.responseHtml;
    html += QStringLiteral("<br><small>(analisi map-reduce su tutto il log)</small>");
    html += buildUserFilterContextHtml();
    form->appendMessage("AI Assistant", html);
    if (!result.indices.isEmpty()) {
        form->setResults(result.indices);
        highlightIndices(result.indices);
    }
}

void DltChatPlugin::onMapReduceFailed(const QString &stage, const QString &reason)
{
    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = false;
        m_pendingAiIsMapReduce = false;
    }
    if (!form) return;
    form->setProcessingProgress(false);

    AiErrorReporter::Context ctx;
    if (m_llmAnalyzer) {
        ctx.provider = m_llmAnalyzer->detectProviderType();
        ctx.model    = m_llmAnalyzer->modelName();
    }
    ctx.shardTotal = m_hierStore.blocks().size();
    {
        QMutexLocker lk(&entriesMutex);
        ctx.entryCount = entries.size();
    }
    form->appendMessage("AI Assistant",
        AiErrorReporter::formatHtml(stage, reason, ctx));
}

void DltChatPlugin::onIngestionReady()
{
    const auto stats = m_hierStore.statistics();
    updateStatus(QString("AI index ready: %1 entries, %2 ECUs, %3 blocks")
                     .arg(stats.totalEntries)
                     .arg(stats.byEcu.size())
                     .arg(m_hierStore.blocks().size()));
}

void DltChatPlugin::onIngestionStageProgress(const QString &stage, int pct)
{
    updateStatus(QString("AI indexing %1: %2%").arg(stage).arg(pct));
}

void DltChatPlugin::onIngestionFailed(const QString &stage, const QString &reason)
{
    // Ingestion failure does not block normal use — AI queries fall back to
    // single-shot retrieval without the panoramic digest. Still report it.
    if (!form) return;
    AiErrorReporter::Context ctx;
    {
        QMutexLocker lk(&entriesMutex);
        ctx.entryCount = entries.size();
    }
    ctx.hint = QStringLiteral("AI queries will still work without the panoramic digest; "
                              "consider reducing blockSize in dlt_chat_plugin.ini.");
    form->appendMessage("AI Assistant",
        AiErrorReporter::formatHtml(stage, reason, ctx));
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(dltchatplugin, DltChatPlugin);
#endif


