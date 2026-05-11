#include "dltchatplugin.h"

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QMutexLocker>

DltChatPlugin::DltChatPlugin()
    : form(nullptr)
    , dltFile(nullptr)
    , mainTableView(nullptr)
    , messageDecoder(nullptr)
    , highlightColor(255, 230, 128)
    , m_analyzer(nullptr)
    , m_ruleBasedAnalyzer(nullptr)
    , m_llmAnalyzer(nullptr)
    , m_currentAnalyzerType("rule-based")
{
    setupDefaultAnalyzer();
}

DltChatPlugin::~DltChatPlugin()
{
    delete m_llmAnalyzer;
}

void DltChatPlugin::setupDefaultAnalyzer()
{
    m_ruleBasedAnalyzer = new DltRuleBasedAnalyzer();
    m_analyzer = m_ruleBasedAnalyzer;
}

QString DltChatPlugin::name()
{
    return QString("DLT Log Assistant");
}

QString DltChatPlugin::pluginVersion()
{
    return DLT_CHAT_PLUGIN_VERSION;
}

QString DltChatPlugin::pluginInterfaceVersion()
{
    return PLUGIN_INTERFACE_VERSION;
}

QString DltChatPlugin::description()
{
    return QString("Chat-based log analysis for DLT Viewer with Rule-Based and LLM support");
}

QString DltChatPlugin::error()
{
    return errorText;
}

bool DltChatPlugin::loadConfig(QString /*filename*/)
{
    return true;
}

bool DltChatPlugin::saveConfig(QString /*filename*/)
{
    return true;
}

QStringList DltChatPlugin::infoConfig()
{
    QStringList info;
    info << QString("Analyzer: %1").arg(m_currentAnalyzerType);
    if (m_analyzer)
    {
        info << m_analyzer->configurationInfo();
    }
    return info;
}

QWidget* DltChatPlugin::initViewer()
{
    form = new DltChat::Form();
    connect(form, &DltChat::Form::querySubmitted, this, &DltChatPlugin::onQuerySubmitted);
    connect(form, &DltChat::Form::indexActivated, this, &DltChatPlugin::onIndexActivated);
    connect(form, &DltChat::Form::clearHighlightsRequested, this, &DltChatPlugin::onClearHighlightsRequested);
    connect(form, &DltChat::Form::exportRequested, this, &DltChatPlugin::onExportRequested);
    connect(form, &DltChat::Form::exportAllRequested, this, &DltChatPlugin::onExportAllRequested);
    connect(this, &DltChatPlugin::statusChanged, form, &DltChat::Form::setStatusText, Qt::QueuedConnection);

    return form;
}

void DltChatPlugin::initFileStart(QDltFile *file)
{
    dltFile = file;
    clearData();
    updateStatus("Caricamento log in corso...");
}

void DltChatPlugin::initFileFinish()
{
    int count = 0;
    {
        QMutexLocker locker(&entriesMutex);
        count = entries.size();
    }
    updateStatus(QString("Log caricato: %1 messaggi. [Analyzer: %2]")
        .arg(count)
        .arg(m_currentAnalyzerType));
}

void DltChatPlugin::initMsg(int index, QDltMsg &msg)
{
    ingestMessage(index, msg);
}

void DltChatPlugin::initMsgDecoded(int index, QDltMsg &msg)
{
    ingestMessage(index, msg);
}

void DltChatPlugin::updateFileStart()
{
}

void DltChatPlugin::updateMsg(int index, QDltMsg &msg)
{
    ingestMessage(index, msg);
}

void DltChatPlugin::updateMsgDecoded(int index, QDltMsg &msg)
{
    ingestMessage(index, msg);
}

void DltChatPlugin::updateFileFinish()
{
    int count = 0;
    {
        QMutexLocker locker(&entriesMutex);
        count = entries.size();
    }
    updateStatus(QString("Log aggiornato: %1 messaggi.").arg(count));
}

void DltChatPlugin::selectedIdxMsg(int /*index*/, QDltMsg &/*msg*/)
{
}

void DltChatPlugin::selectedIdxMsgDecoded(int /*index*/, QDltMsg &/*msg*/)
{
}

bool DltChatPlugin::initControl(QDltControl * /*control*/)
{
    return true;
}

bool DltChatPlugin::initConnections(QStringList /*list*/)
{
    return true;
}

bool DltChatPlugin::controlMsg(int /*index*/, QDltMsg &/*msg*/)
{
    return true;
}

bool DltChatPlugin::stateChanged(int /*index*/, QDltConnection::QDltConnectionState /*connectionState*/, QString /*hostname*/)
{
    return true;
}

bool DltChatPlugin::autoscrollStateChanged(bool /*enabled*/)
{
    return true;
}

void DltChatPlugin::initMessageDecoder(QDltMessageDecoder* pMessageDecoder)
{
    messageDecoder = pMessageDecoder;
}

void DltChatPlugin::initMainTableView(QTableView* pTableView)
{
    mainTableView = pTableView;
}

void DltChatPlugin::configurationChanged()
{
}

void DltChatPlugin::setAnalyzerType(const QString &type)
{
    if (type == "llm" && m_llmAnalyzer && m_llmAnalyzer->isAvailable())
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

QString DltChatPlugin::currentAnalyzerType() const
{
    return m_currentAnalyzerType;
}

void DltChatPlugin::configureLlmAnalyzer(const QString &endpoint, const QString &apiKey, const QString &model)
{
    if (!m_llmAnalyzer)
    {
        m_llmAnalyzer = new DltLlmAnalyzerInterface(this);
    }

    m_llmAnalyzer->setApiEndpoint(endpoint);
    m_llmAnalyzer->setApiKey(apiKey);
    m_llmAnalyzer->setModelName(model);
}

void DltChatPlugin::onQuerySubmitted(const QString &query)
{
    if (!form)
    {
        return;
    }

    form->appendMessage("Tu", query.toHtmlEscaped());

    DltAnalyzerInterface::QueryResult result = analyzeQueryInternal(query);

    QString responseHtml = result.responseHtml;
    if (result.processingTimeMs > 0)
    {
        responseHtml += QString("<br><small>Tempo: %1ms</small>").arg(result.processingTimeMs);
    }

    form->appendMessage("DLT Assistant", responseHtml);
    form->setResults(result.indices, result.snippets);
    highlightIndices(result.indices);
}

void DltChatPlugin::onIndexActivated(int index)
{
    if (!mainTableView || !dltFile)
    {
        if (form)
        {
            form->appendMessage("DLT Assistant", "Navigazione non disponibile: vista principale non trovata.");
        }
        return;
    }

    int row = findRowForIndex(index);
    if (row < 0)
    {
        if (form)
        {
            form->appendMessage("DLT Assistant", "Indice non trovato nella vista corrente (filtri attivi?).");
        }
        return;
    }

    QModelIndex targetIndex = mainTableView->model()->index(row, 0);
    if (!targetIndex.isValid())
    {
        return;
    }

    if (mainTableView->selectionModel())
    {
        mainTableView->selectionModel()->setCurrentIndex(
            targetIndex,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    mainTableView->scrollTo(targetIndex, QAbstractItemView::PositionAtCenter);
}

void DltChatPlugin::onClearHighlightsRequested()
{
    highlightIndices(QList<int>());
    if (form)
    {
        form->setResults(QList<int>(), QStringList());
    }
}

void DltChatPlugin::onExportRequested(const QString &filePath, const QList<int> &indices, const QStringList &snippets, const QString &query)
{
    Q_UNUSED(query);

    QVector<DltChatAnalyzer::LogEntry> snapshot;
    {
        QMutexLocker locker(&entriesMutex);
        snapshot = entries;
    }

    bool success = DltExport::exportToCsv(filePath, indices, snippets, query);

    if (form)
    {
        if (success)
        {
            form->appendMessage("DLT Assistant", QString("Risultati esportati in: %1").arg(filePath));
        }
        else
        {
            form->appendMessage("DLT Assistant", "Errore durante l'esportazione CSV.");
        }
    }
}

void DltChatPlugin::onExportAllRequested(const QString &filePath)
{
    QVector<DltChatAnalyzer::LogEntry> snapshot;
    {
        QMutexLocker locker(&entriesMutex);
        snapshot = entries;
    }

    bool success = DltExport::exportAllEntries(filePath, snapshot);

    if (form)
    {
        if (success)
        {
            form->appendMessage("DLT Assistant", QString("Tutti i log esportati in: %1 (%2 entries)")
                .arg(filePath)
                .arg(snapshot.size()));
        }
        else
        {
            form->appendMessage("DLT Assistant", "Errore durante l'esportazione CSV.");
        }
    }
}

void DltChatPlugin::clearData()
{
    QMutexLocker locker(&entriesMutex);
    entries.clear();
    indexToPos.clear();
}

void DltChatPlugin::ingestMessage(int index, QDltMsg &msg)
{
    if (!dltFile)
    {
        return;
    }

    QMutexLocker locker(&entriesMutex);
    if (indexToPos.contains(index))
    {
        return;
    }

    QDltMsg localMsg = msg;
    if (messageDecoder)
    {
        messageDecoder->decodeMsg(localMsg, 0);
    }

    DltChatAnalyzer::LogEntry entry;
    entry.index = index;
    entry.time = QString("%1.%2")
        .arg(localMsg.getTimeString())
        .arg(localMsg.getMicroseconds(), 6, 10, QLatin1Char('0'));
    entry.timestamp = QString("%1.%2")
        .arg(localMsg.getTimestamp() / 10000)
        .arg(localMsg.getTimestamp() % 10000, 4, 10, QLatin1Char('0'));
    entry.ecu = localMsg.getEcuid();
    entry.apid = localMsg.getApid();
    entry.ctid = localMsg.getCtid();
    entry.level = localMsg.getSubtypeString().toLower();
    entry.payload = DltChatAnalyzer::simplifyPayload(localMsg.toStringPayload());

    indexToPos.insert(index, entries.size());
    entries.append(entry);
}

DltAnalyzerInterface::QueryResult DltChatPlugin::analyzeQueryInternal(const QString &query)
{
    if (!m_analyzer)
    {
        DltAnalyzerInterface::QueryResult fallback;
        fallback.responseHtml = "Nessun analyzer disponibile.";
        fallback.success = false;
        return fallback;
    }

    QVector<DltChatAnalyzer::LogEntry> snapshot;
    {
        QMutexLocker locker(&entriesMutex);
        snapshot = entries;
    }

    QVector<DltAnalyzerInterface::LogEntry> interfaceEntries;
    interfaceEntries.reserve(snapshot.size());
    for (const auto &entry : snapshot)
    {
        DltAnalyzerInterface::LogEntry ifaceEntry;
        ifaceEntry.index = entry.index;
        ifaceEntry.time = entry.time;
        ifaceEntry.timestamp = entry.timestamp;
        ifaceEntry.ecu = entry.ecu;
        ifaceEntry.apid = entry.apid;
        ifaceEntry.ctid = entry.ctid;
        ifaceEntry.level = entry.level;
        ifaceEntry.payload = entry.payload;
        interfaceEntries.append(ifaceEntry);
    }

    return m_analyzer->analyzeQuery(query, interfaceEntries);
}

void DltChatPlugin::highlightIndices(const QList<int> &indices)
{
    if (!dltFile)
    {
        return;
    }

    QList<unsigned long int> markerRows;
    markerRows.reserve(indices.size());
    for (int idx : indices)
    {
        if (idx >= 0)
        {
            markerRows.append(static_cast<unsigned long int>(idx));
        }
    }

    dltFile->setManualMarkerIndices(markerRows);

    if (mainTableView)
    {
        mainTableView->viewport()->update();
    }
}

int DltChatPlugin::findRowForIndex(int index) const
{
    if (!dltFile)
    {
        return -1;
    }

    const int rows = dltFile->sizeFilter();
    for (int row = 0; row < rows; ++row)
    {
        if (dltFile->getMsgFilterPos(row) == index)
        {
            return row;
        }
    }

    return -1;
}

void DltChatPlugin::updateStatus(const QString &text)
{
    emit statusChanged(text);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(dltchatplugin, DltChatPlugin);
#endif
