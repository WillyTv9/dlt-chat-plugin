#include "dltchatplugin.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QRegularExpression>
#include <QItemSelectionModel>
#include <QMutexLocker>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>

static constexpr int kMaxFilterResults = 1000;
static constexpr int kAIPreFilterMax = 100;
static constexpr int kSnippetLength = 120;
static constexpr int kPayloadTruncateAt = 500;
static constexpr int kPreviewCount = 20;
static constexpr int kAICacheMaxEntries = 10000;
static constexpr int kAIDebounceMs = 500;
static constexpr int kMaxEntriesLimit = 500000;

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

void DltChatPlugin::onBulkError(const QString &error)
{
    m_bulkAnalysisInProgress = false;
    updateStatus(QString("Bulk analysis error: %1").arg(error));
}

static QStringList buildLevels(const QHash<int,int> &idxMap,
                                const QVector<DltAnalyzerInterface::LogEntry> &ents,
                                const QList<int> &indices)
{
    QStringList l; l.reserve(indices.size());
    for (int idx : indices) {
        int p = idxMap.value(idx, -1);
        l.append(p >= 0 && p < ents.size() ? ents[p].level : QString());
    }
    return l;
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
    QString tagsUrl = m_llmAnalyzer->apiEndpoint();
    tagsUrl.replace("/api/generate", "/api/tags");

    QNetworkReply *reply = mgr->get(QNetworkRequest(QUrl(tagsUrl)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, mgr]() {
        reply->deleteLater();
        mgr->deleteLater();

        if (reply->error() == QNetworkReply::NoError)
        {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject())
            {
                QJsonArray models = doc.object()["models"].toArray();
                for (const auto &m : models)
                {
                    QString name = m.toObject()["name"].toString();
                    if (name.startsWith(m_llmAnalyzer->modelName()))
                    {
                        m_aiAvailabilityRetryCount = 0;
                        setAiState(2, m_llmAnalyzer->modelName());
                        return;
                    }
                }
            }
            setAiState(1, m_llmAnalyzer->modelName());
        }
        else
        {
            setAiState(1, m_llmAnalyzer->modelName());
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
    emit onAiAvailabilityChanged(state, m_aiModelName);
}

int DltChatPlugin::aiAvailabilityBackoffMs() const
{
    if (m_aiAvailabilityRetryCount <= 1) return 5000;
    if (m_aiAvailabilityRetryCount <= 3) return 15000;
    return 60000;
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
    if (!ep.isEmpty()) configureLlmAnalyzer(ep, key, model);
    m_bulkAnalysisEnabled = settings.value("bulkAnalysisEnabled", false).toBool();
    settings.endGroup();
    settings.beginGroup("Behavior");
    if (settings.contains("highlightColor"))
        highlightColor = QColor(settings.value("highlightColor").toString());
    settings.endGroup();
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
    }
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

    applyConfigToForm();
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
    filterRowMapDirty = true;
    updateStatus("Loading log file...");
}

void DltChatPlugin::initFileFinish()
{
    rebuildFilterRowMap();
    updateDomainStatus();

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

void DltChatPlugin::selectedIdxMsg(int, QDltMsg &) {}
void DltChatPlugin::selectedIdxMsgDecoded(int, QDltMsg &) {}

bool DltChatPlugin::initControl(QDltControl *) { return true; }
bool DltChatPlugin::initConnections(QStringList) { return true; }
bool DltChatPlugin::controlMsg(int, QDltMsg &) { return true; }
bool DltChatPlugin::stateChanged(int, QDltConnection::QDltConnectionState, QString) { return true; }
bool DltChatPlugin::autoscrollStateChanged(bool) { return true; }
void DltChatPlugin::initMessageDecoder(QDltMessageDecoder *p) { messageDecoder = p; }
void DltChatPlugin::initMainTableView(QTableView *p) { mainTableView = p; }
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

void DltChatPlugin::onQuerySubmitted(const QString &query)
{
    if (!form) return;
    form->appendMessage("Tu", query.toHtmlEscaped());

    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }

    if (snapshot.isEmpty()) {
        form->appendMessage("Chat Assistant", "Nessun log caricato. Apri un file DLT.");
        form->setResults(QList<int>(), QStringList(), QStringList());
        return;
    }

    QElapsedTimer timer;
    timer.start();

    QString lq = query.trimmed().toLower();

    // --- ROUTING ---

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
        QList<int> display = indices.mid(0, kMaxDisplayResults);
        form->setResults(display, QStringList(), QStringList());
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
        QList<int> display = indices.mid(0, kMaxDisplayResults);
        form->setResults(display, QStringList(), QStringList());
        highlightIndices(indices);
        return;
    }

    // 1. Quick Button / Preset automotive match
    static const QHash<QString, QString> presetMap = {
        {"carplay", "carplay"},
        {"androidauto", "androidauto"},
        {"video_focus", "video_focus"},
        {"audio_ducking", "audio_ducking"},
        {"mdns", "mdns"},
        {"sensor_data", "sensor_data"},
        {"auth_errors", "auth_errors"},
        {"session", "session"},
    };

    auto presetIt = presetMap.constFind(lq);
    if (presetIt != presetMap.constEnd()) {
        QString presetName = presetIt.value();
        auto filtered = AutomotiveLogParser::filterByPreset(snapshot, presetName);
        DltAnalyzerInterface::QueryResult result;
        if (filtered.isEmpty()) {
            result.responseHtml = QString("Nessun messaggio <b>%1</b> trovato.").arg(presetName);
            result.success = true;
        } else {
            result = m_ruleBasedAnalyzer->analyzeQuery(query, filtered);
        }
        result.processingTimeMs = timer.elapsed();
        {
            int totalIdx = result.indices.size();
            int displayIdx = qMin(totalIdx, kMaxDisplayResults);
            if (totalIdx > displayIdx) {
                result.snippets = result.snippets.mid(0, displayIdx);
                result.responseHtml += QString("<br><em>Mostrati %1 su %2 risultati totali.</em>")
                    .arg(displayIdx).arg(totalIdx);
            }
        }
        QString html = result.responseHtml;
        html += buildUserFilterContextHtml();
        if (result.processingTimeMs > 0)
            html += QString("<br><small>%1ms</small>").arg(result.processingTimeMs);
        form->appendMessage("Chat Assistant", html);
        {
            QMutexLocker lk(&entriesMutex);
            int displayIdx = qMin(result.indices.size(), kMaxDisplayResults);
            QList<int> displayIndices = result.indices.mid(0, displayIdx);
            QStringList displaySnippets = result.snippets.mid(0, displayIdx);
            QStringList lvls = buildLevels(indexToPos, entries, displayIndices);
            form->setResults(displayIndices, displaySnippets, lvls);
        }
        highlightIndices(result.indices);
        return;
    }

    // 2. Check if this is a special command for the rule-based analyzer
    bool isSpecial = false;
    const QStringList specialCommands = {
        "summary", "riassumi", "sintesi", "statistiche",
        "timeline", "cronologia",
        "help", "aiuto", "comandi",
        "keywords", "categorie", "categories",
        "pattern",
        "categorizza", "categorize", "classifica"
    };
    for (const auto &cmd : specialCommands) {
        if (lq == cmd || lq.startsWith(cmd + " ")) { isSpecial = true; break; }
    }
    // Level-only queries
    if (lq.contains("error") || lq.contains("errore") || lq.contains("fatal") ||
        lq.contains("warn") || lq == "info" || lq == "debug" || lq == "verbose")
        isSpecial = true;
    // Category queries
    const QStringList categories = {"can", "security", "memory", "performance",
                                     "diagnostic", "gps"};
    for (const auto &cat : categories) {
        if (lq.contains(cat)) { isSpecial = true; break; }
    }

    if (!isSpecial) {
        // Live Search — process ALL entries, display capped
        QList<int> searchIndices = liveSearch(lq);
        int totalResults = searchIndices.size();

        // Build snippets for display only (keep performance)
        QStringList snippets;
        int displayCount = qMin(totalResults, kMaxDisplayResults);
        snippets.reserve(displayCount);
        {
            QMutexLocker lk(&entriesMutex);
            for (int i = 0; i < displayCount; ++i) {
                int pos = indexToPos.value(searchIndices[i], -1);
                if (pos >= 0 && pos < entries.size())
                    snippets.append(entries[pos].payload.left(kSnippetLength));
                else
                    snippets.append(QString());
            }
        }

        QString html;
        if (searchIndices.isEmpty()) {
            html = "Nessun risultato per <b>" + query.toHtmlEscaped() + "</b>.<br>"
                   "Suggerimenti: prova <b>riassumi</b> per statistiche, o un livello (error/warn/info/debug).";
        } else {
            html = QString("Trovati <b>%1</b> risultati per <b>%2</b> su %3 totali.<br>")
                .arg(totalResults).arg(query.toHtmlEscaped()).arg(snapshot.size());
            QStringList preview;
            for (int i = 0; i < qMin(20, totalResults); ++i)
                preview.append(QString::number(searchIndices[i]));
            html += QString("Primi indici: %1").arg(preview.join(", "));
            if (totalResults > 20)
                html += QString(" (+%1 totali)").arg(totalResults - 20);
            if (totalResults > displayCount)
                html += QString("<br><em>Mostrati %1 su %2 risultati.</em>").arg(displayCount).arg(totalResults);
        }
        html += buildUserFilterContextHtml();
        html += QString("<br><small>%1ms</small>").arg(timer.elapsed());
        form->appendMessage("Chat Assistant", html);
        {
            QMutexLocker lk(&entriesMutex);
            QStringList lvls = buildLevels(indexToPos, entries, searchIndices.mid(0, displayCount));
            form->setResults(searchIndices.mid(0, displayCount), snippets, lvls);
        }
        // Highlight ALL indices (not just displayed ones)
        highlightIndices(searchIndices);
        return;
    }

    // 3. Special commands delegate to rule-based analyzer (process ALL, display capped)
    DltAnalyzerInterface::QueryResult result = m_ruleBasedAnalyzer->analyzeQuery(query, snapshot);
    result.processingTimeMs = timer.elapsed();
    {
        int totalIdx = result.indices.size();
        int displayIdx = qMin(totalIdx, kMaxDisplayResults);
        if (totalIdx > displayIdx)
            result.responseHtml += QString("<br><em>Mostrati %1 su %2 risultati totali.</em>")
                .arg(displayIdx).arg(totalIdx);
    }
    QString html = result.responseHtml;
    html += buildUserFilterContextHtml();
    if (result.processingTimeMs > 0)
        html += QString("<br><small>%1ms</small>").arg(result.processingTimeMs);
    form->appendMessage("Chat Assistant", html);
    {
        QMutexLocker lk(&entriesMutex);
        int displayIdx = qMin(result.indices.size(), kMaxDisplayResults);
        QList<int> displayIndices = result.indices.mid(0, displayIdx);
        QStringList displaySnippets = result.snippets.mid(0, displayIdx);
        QStringList lvls = buildLevels(indexToPos, entries, displayIndices);
        form->setResults(displayIndices, displaySnippets, lvls);
    }
    highlightIndices(result.indices);
}

void DltChatPlugin::onAiQuerySubmitted(const QString &query)
{
    if (!form) return;

    // Rate limiting guard
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

    if (m_aiState != 2 || !m_llmAnalyzer)
    {
        form->setProcessingProgress(false);
        QElapsedTimer timer; timer.start();
        QStringList keywords = extractKeywords(query);
        QVector<DltAnalyzerInterface::LogEntry> pre;
        if (!keywords.isEmpty())
        {
            QSet<int> s = invertedIndex.value(keywords[0]);
            for (int k = 1; k < keywords.size() && !s.isEmpty(); ++k)
                s.intersect(invertedIndex.value(keywords[k]));
            for (const auto &e : snapshot)
                if (s.contains(e.index)) pre.append(e);
        }
        else { pre = snapshot; }
        if (pre.size() > kAIPreFilterMax) pre.resize(kAIPreFilterMax);

        auto result = m_ruleBasedAnalyzer->analyzeQuery(query, pre.isEmpty() ? snapshot : pre);
        result.processingTimeMs = timer.elapsed();
        QString html = result.responseHtml + "<br><em>AI non disponibile, analisi locale.</em>";
        html += buildUserFilterContextHtml();
        form->appendMessage("AI Assistant (fallback)", html);
        form->setResults(result.indices, result.snippets,
            buildLevels(indexToPos, entries, result.indices));
        highlightIndices(result.indices);
        return;
    }

    QStringList keywords = extractKeywords(query);
    QVector<DltAnalyzerInterface::LogEntry> prefiltered;
    if (!keywords.isEmpty())
    {
        QSet<int> s = invertedIndex.value(keywords[0]);
        for (int k = 1; k < keywords.size() && !s.isEmpty(); ++k)
            s.intersect(invertedIndex.value(keywords[k]));

        for (const auto &e : snapshot)
        {
            if (s.contains(e.index) || keywords.isEmpty())
                prefiltered.append(e);
            if (prefiltered.size() >= kMaxFilterResults) break;
        }
    }

    if (prefiltered.isEmpty()) prefiltered = snapshot;
    if (prefiltered.size() > kAIPreFilterMax) prefiltered.resize(kAIPreFilterMax);

    // Check cache
    {
        QMutexLocker lk(&m_llmMutex);
        QString cacheKey = buildAiCacheKey(query, prefiltered);
        auto it = m_aiResponseCache.find(cacheKey);
        if (it != m_aiResponseCache.end()) {
            form->setProcessingProgress(false);
            const auto &cached = it.value();
            QString html = cached.responseHtml + "<br><small>(risposta cache)</small>";
            html += buildUserFilterContextHtml();
            form->appendMessage("AI Assistant", html);
            form->setResults(cached.indices, cached.snippets,
                buildLevels(indexToPos, entries, cached.indices));
            highlightIndices(cached.indices);
            return;
        }
    }

    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = true;
        m_llmRequestTimer.start();
    }

    // Add user filter context to the LLM analyzer
    QString filterContext;
    if (m_userFilterManager && m_userFilterManager->activeFilterCount() > 0) {
        QStringList activeFilters;
        for (const auto &f : m_userFilterManager->filters()) {
            if (f.enabled && f.isValid)
                activeFilters.append(QString("%1 (pattern: %2)")
                    .arg(f.label, f.regex.pattern()));
        }
        if (!activeFilters.isEmpty())
            filterContext = "\nUser-defined active filters:\n" + activeFilters.join("\n");
    }

    m_llmAnalyzer->setExtraContext(filterContext);
    m_llmAnalyzer->analyzeQueryAsync(query, prefiltered);
}

void DltChatPlugin::onLlmResultReady(const DltAnalyzerInterface::QueryResult &result, const QString &originalQuery)
{
    if (!form) return;
    form->setProcessingProgress(false);

    {
        QMutexLocker lk(&m_llmMutex);
        m_llmRequestInProgress = false;
    }

    // Cache the result
    if (result.success) {
        QMutexLocker lk(&m_llmMutex);
        QString cacheKey = buildAiCacheKey(originalQuery,
            QVector<DltAnalyzerInterface::LogEntry>());
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
        form->setResults(fb.indices, fb.snippets,
            buildLevels(indexToPos, entries, fb.indices));
        highlightIndices(fb.indices);
        return;
    }

    html += buildUserFilterContextHtml();
    form->appendMessage("AI Assistant", html);
    form->setResults(result.indices, result.snippets,
        buildLevels(indexToPos, entries, result.indices));
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

    if (dlg.exec() == QDialog::Accepted)
    {
        m_llmAnalyzer->setApiEndpoint(dlg.endpoint());
        m_llmAnalyzer->setApiKey(dlg.apiKey());
        m_llmAnalyzer->setModelName(dlg.model());
        m_llmAnalyzer->setMaxTokens(dlg.maxTokens());
        m_llmAnalyzer->setTemperature(dlg.temperature());
        m_llmAnalyzer->setTimeout(dlg.timeoutMs());
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
    highlightIndices(QList<int>());
    if (form) form->setResults(QList<int>(), QStringList(), QStringList());
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
    if (entries.size() >= kMaxEntriesLimit) return;

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
    for (const QString &k : kw)
        invertedIndex[k].insert(index);
}

void DltChatPlugin::rebuildFilterRowMap()
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
        const_cast<DltChatPlugin*>(this)->rebuildFilterRowMap();
    return filterRowMap.value(index, -1);
}

void DltChatPlugin::highlightIndices(const QList<int> &indices)
{
    if (!dltFile) return;
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
    dltFile->setManualMarkerIndices(m);
    if (mainTableView) mainTableView->viewport()->update();
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
    int total = entries.size();

    if (kw.isEmpty()) {
        // Fallback: regex search on ALL entries (no limit)
        QString escaped = QRegularExpression::escape(rawQuery);
        QRegularExpression rx(escaped, QRegularExpression::CaseInsensitiveOption);
        if (!rx.isValid()) return {};

        QList<int> results;
        results.reserve(total / 10);
        for (const auto &e : entries) {
            bool match = e.payload.contains(rx) ||
                         e.apid.contains(rx) ||
                         e.ctid.contains(rx) ||
                         e.ecu.contains(rx) ||
                         e.level.contains(rx) ||
                         e.domain.contains(rx);
            if (match)
                results.append(e.index);
        }
        return results;
    }

    // Use inverted index: intersect for AND semantics
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

    // Filter candidates by additional fields (NO limit)
    QList<int> results;
    results.reserve(qMin(candidates.size(), 10000));
    QRegularExpression rx(QRegularExpression::escape(rawQuery),
                          QRegularExpression::CaseInsensitiveOption);

    for (const auto &e : entries) {
        if (!candidates.contains(e.index)) continue;

        bool match = e.payload.contains(rx) ||
                     e.apid.contains(rx) ||
                     e.ctid.contains(rx) ||
                     e.ecu.contains(rx) ||
                     e.level.contains(rx) ||
                     e.domain.contains(rx) ||
                     e.time.contains(rx);
        if (match)
            results.append(e.index);
    }

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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(dltchatplugin, DltChatPlugin);
#endif
