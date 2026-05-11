#include "dltchatplugin.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QRegularExpression>
#include <QItemSelectionModel>
#include <QMutexLocker>
#include <QSettings>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

static constexpr int kMaxFilterResults = 200;
static constexpr int kAIPreFilterMax = 100;
static constexpr int kSnippetLength = 120;
static constexpr int kPayloadTruncateAt = 500;
static constexpr int kPreviewCount = 20;

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
{
    indexStopwords.reserve(70);
    for (const char *w : STOPWORDS)
        indexStopwords.insert(QString::fromLatin1(w));
    setupDefaultAnalyzer();
}

DltChatPlugin::~DltChatPlugin()
{
    delete m_llmAnalyzer;
    delete m_ruleBasedAnalyzer;
}

void DltChatPlugin::setupDefaultAnalyzer()
{
    m_ruleBasedAnalyzer = new DltRuleBasedAnalyzer();
    m_analyzer = m_ruleBasedAnalyzer;

    m_llmAnalyzer = DltLlmAnalyzerFactory::createOllamaAnalyzer(
        "http://localhost:11434", "qwen3.5:4b", this);

    connect(m_llmAnalyzer, &DltLlmAnalyzerInterface::queryResultReady,
            this, &DltChatPlugin::onLlmResultReady);

    checkAiAvailabilityAsync();
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
    if (state == 2) m_aiModelName = modelName;
    else if (state == 1) m_aiModelName = modelName.isEmpty() ? m_llmAnalyzer->modelName() : modelName;
    else m_aiModelName.clear();
    m_aiAvailabilityTimer.start();
    emit onAiAvailabilityChanged(state, m_aiModelName);
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
    QString model = settings.value("llmModel", "qwen3.5:4b").toString();
    if (!ep.isEmpty()) configureLlmAnalyzer(ep, key, model);
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
    int count = 0;
    { QMutexLocker l(&entriesMutex); count = entries.size(); }
    updateStatus(QString("Loaded %1 messages. [%2]").arg(count).arg(m_currentAnalyzerType));
}

void DltChatPlugin::initMsg(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }
void DltChatPlugin::initMsgDecoded(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }
void DltChatPlugin::updateFileStart() {}
void DltChatPlugin::updateMsg(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }
void DltChatPlugin::updateMsgDecoded(int idx, QDltMsg &msg) { ingestMessage(idx, msg); }

void DltChatPlugin::updateFileFinish()
{
    rebuildFilterRowMap();
    int c = 0;
    { QMutexLocker l(&entriesMutex); c = entries.size(); }
    updateStatus(QString("Updated: %1 messages.").arg(c));
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

void DltChatPlugin::onQuerySubmitted(const QString &query)
{
    if (!form) return;
    form->appendMessage("Tu", query.toHtmlEscaped());

    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }

    QElapsedTimer timer;
    timer.start();

    QStringList keywords = extractKeywords(query);
    QSet<int> matched;
    if (!keywords.isEmpty())
    {
        auto it = invertedIndex.constFind(keywords[0]);
        if (it != invertedIndex.constEnd())
            matched = it.value();
        for (int k = 1; k < keywords.size() && !matched.isEmpty(); ++k)
            matched.intersect(invertedIndex.value(keywords[k]));
    }

    QStringList matchedKw;
    if (query.contains("error") || query.contains("errore")) matchedKw << "error";
    if (query.contains("fatal") || query.contains("fatale")) matchedKw << "fatal";
    if (query.contains("warn")) matchedKw << "warn";
    if (query.contains("info")) matchedKw << "info";
    if (query.contains("debug")) matchedKw << "debug";
    if (query.contains("verbose")) matchedKw << "verbose";

    QVector<DltAnalyzerInterface::LogEntry> filtered;
    if (!matchedKw.isEmpty() || !keywords.isEmpty())
    {
        filtered.reserve(snapshot.size());
        for (const auto &entry : snapshot)
        {
            if (!matchedKw.isEmpty() && !matchedKw.contains(entry.level)) continue;
            if (!matched.isEmpty() && !matched.contains(entry.index)) continue;
            filtered.append(entry);
            if (filtered.size() >= kMaxFilterResults) break;
        }
    }
    else filtered = snapshot;

    DltAnalyzerInterface::QueryResult result;
    if (filtered.isEmpty())
    {
        result.responseHtml = query.contains("summary") || query.contains("riassumi")
            ? m_ruleBasedAnalyzer->analyzeQuery(query, snapshot).responseHtml
            : "Nessun risultato. Prova altre parole chiave o 'riassumi'.";
        result.success = !result.responseHtml.isEmpty();
    }
    else
    {
        result = m_ruleBasedAnalyzer->analyzeQuery(query, filtered);
    }

    result.processingTimeMs = timer.elapsed();
    QString html = result.responseHtml;
    if (result.processingTimeMs > 0)
        html += QString("<br><small>%1ms</small>").arg(result.processingTimeMs);

    form->appendMessage("Chat Assistant", html);
    {
        QMutexLocker lk(&entriesMutex);
        QStringList lvls = buildLevels(indexToPos, entries, result.indices);
        form->setResults(result.indices, result.snippets, lvls);
    }
    highlightIndices(result.indices);
}

void DltChatPlugin::onAiQuerySubmitted(const QString &query)
{
    if (!form) return;
    form->appendMessage("Tu (AI)", query.toHtmlEscaped());
    form->setProcessingProgress(true);

    QVector<DltAnalyzerInterface::LogEntry> snapshot;
    { QMutexLocker l(&entriesMutex); snapshot = entries; }

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
        form->appendMessage("AI Assistant (fallback)", result.responseHtml
            + "<br><em>AI non disponibile, analisi locale.</em>");
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

    m_llmAnalyzer->analyzeQueryAsync(query, prefiltered);
}

void DltChatPlugin::onLlmResultReady(const DltAnalyzerInterface::QueryResult &result, const QString &originalQuery)
{
    if (!form) return;
    form->setProcessingProgress(false);

    QString html = result.responseHtml;
    if (!result.success)
    {
        html += "<br><em>Fallback a analisi locale.</em>";
        QVector<DltAnalyzerInterface::LogEntry> snapshot;
        { QMutexLocker l(&entriesMutex); snapshot = entries; }
        auto fb = m_ruleBasedAnalyzer->analyzeQuery(originalQuery, snapshot);
        html = fb.responseHtml + "<br><em>AI non disponibile. Risultato locale.</em>";
        if (fb.processingTimeMs > 0)
            html += QString("<br><small>%1ms</small>").arg(fb.processingTimeMs);
        form->appendMessage("AI Assistant", html);
        form->setResults(fb.indices, fb.snippets,
            buildLevels(indexToPos, entries, fb.indices));
        highlightIndices(fb.indices);
        return;
    }

    form->appendMessage("AI Assistant", html);
    form->setResults(result.indices, result.snippets,
        buildLevels(indexToPos, entries, result.indices));
    highlightIndices(result.indices);
}

void DltChatPlugin::onConfigureAiClicked()
{
    if (!m_llmAnalyzer || !form) return;
    DltAiOptionsDialog dlg(form);
    dlg.setWindowTitle(tr("AI Configuration"));               // Italian: Configurazione AI
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
    QVector<DltAnalyzerInterface::LogEntry> snap;
    { QMutexLocker l(&entriesMutex); snap = entries; }
    bool ok = DltExport::exportToCsv(filePath, indices, snippets, query, snap);
    if (form) form->appendMessage("Chat Assistant",
        ok ? QString("Esportati: %1").arg(filePath) : "Errore esportazione.");
}

void DltChatPlugin::onExportAllRequested(const QString &filePath)
{
    QVector<DltAnalyzerInterface::LogEntry> snap;
    { QMutexLocker l(&entriesMutex); snap = entries; }
    bool ok = DltExport::exportAllEntries(filePath, snap);
    if (form) form->appendMessage("Chat Assistant",
        ok ? QString("Tutti esportati: %1 (%2)").arg(filePath).arg(snap.size()) : "Errore esportazione.");
}

void DltChatPlugin::clearData()
{
    QMutexLocker l(&entriesMutex);
    entries.clear();
    indexToPos.clear();
    invertedIndex.clear();
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

    indexToPos.insert(index, entries.size());
    entries.append(e);

    QStringList kw = extractKeywords(e.payload);
    kw.append(e.level);
    kw.append(e.apid.toLower());
    kw.append(e.ctid.toLower());
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
    QList<unsigned long int> m;
    m.reserve(indices.size());
    for (int i : indices) if (i >= 0) m.append(static_cast<unsigned long>(i));
    dltFile->setManualMarkerIndices(m);
    if (mainTableView) mainTableView->viewport()->update();
}

void DltChatPlugin::updateStatus(const QString &text)
{
    emit statusChanged(text);
}

void DltChatPlugin::onAiAvailabilityChanged(int state, const QString &modelName)
{
    Q_UNUSED(state); Q_UNUSED(modelName);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(dltchatplugin, DltChatPlugin);
#endif
