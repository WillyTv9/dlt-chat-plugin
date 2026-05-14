#ifndef DLTCHATPLUGIN_H
#define DLTCHATPLUGIN_H

#include <QObject>
#include <QColor>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QTableView>
#include <QElapsedTimer>

#include "plugininterface.h"
#include "chatform.h"

#include "dltexport.h"
#include "dltanalyzerinterface.h"
#include "dltllmanalyzerinterface.h"
#include "dltaioptionsdialog.h"
#include "automotivelogparser.h"
#include "contextualextractor.h"
#include "conversationmanager.h"
#include "fibexenricher.h"
#include "userfiltermanager.h"
#include "dltbulkanalyzer.h"
#include "qdltmessagedecoder.h"
#include "qdltfile.h"

#define DLT_CHAT_PLUGIN_VERSION "0.5.0"

class DltChatPlugin : public QObject, QDLTPluginInterface, QDltPluginViewerInterface, QDltPluginControlInterface
{
    Q_OBJECT
    Q_INTERFACES(QDLTPluginInterface)
    Q_INTERFACES(QDltPluginViewerInterface)
    Q_INTERFACES(QDltPluginControlInterface)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.genivi.DLT.DltChatPlugin")
#endif

public:
    DltChatPlugin();
    ~DltChatPlugin();

    QString name() override;
    QString pluginVersion() override;
    QString pluginInterfaceVersion() override;
    QString description() override;
    QString error() override;
    bool loadConfig(QString filename) override;
    bool saveConfig(QString filename) override;
    QStringList infoConfig() override;

    QWidget* initViewer() override;
    void initFileStart(QDltFile *file) override;
    void initFileFinish() override;
    void initMsg(int index, QDltMsg &msg) override;
    void initMsgDecoded(int index, QDltMsg &msg) override;
    void updateFileStart() override;
    void updateMsg(int index, QDltMsg &msg) override;
    void updateMsgDecoded(int index, QDltMsg &msg) override;
    void updateFileFinish() override;
    void selectedIdxMsg(int index, QDltMsg &msg) override;
    void selectedIdxMsgDecoded(int index, QDltMsg &msg) override;

    bool initControl(QDltControl *control) override;
    bool initConnections(QStringList list) override;
    bool controlMsg(int index, QDltMsg &msg) override;
    bool stateChanged(int index, QDltConnection::QDltConnectionState connectionState, QString hostname) override;
    bool autoscrollStateChanged(bool enabled) override;
    void initMessageDecoder(QDltMessageDecoder* pMessageDecoder) override;
    void initMainTableView(QTableView* pTableView) override;
    void configurationChanged() override;

    void setAnalyzerType(const QString &type);
    QString currentAnalyzerType() const;
    void configureLlmAnalyzer(const QString &endpoint, const QString &apiKey, const QString &model);
    bool loadFibexFile(const QString &filePath, QString *errorOut = nullptr);
    bool isFibexLoaded() const { return m_fibexEnricher.isLoaded(); }

signals:
    void statusChanged(const QString &text);
    void onAiAvailabilityChanged(int state, const QString &modelName);

private slots:
    void onQuerySubmitted(const QString &query);
    void onAiQuerySubmitted(const QString &query);
    void onConfigureAiClicked();
    void onIndexActivated(int index);
    void onClearHighlightsRequested();
    void onExportRequested(const QString &filePath, const QList<int> &indices, const QStringList &snippets, const QString &query);
    void onExportAllRequested(const QString &filePath);
    void onLlmResultReady(const DltAnalyzerInterface::QueryResult &result, const QString &originalQuery);
    void onUserFilterLoadRequested(const QString &path);
    void onBulkProgress(double progress, int processed, int total);
    void onBulkFinished(bool success);
    void onBulkError(const QString &error);

private:
    void clearData();
    void ingestMessage(int index, QDltMsg &msg);
    void rebuildFilterRowMap();
    void highlightIndices(const QList<int> &indices);
    int findRowForIndex(int index) const;
    void updateStatus(const QString &text);
    void setupDefaultAnalyzer();
    void applyConfigToForm();
    void checkAiAvailabilityAsync();
    QStringList extractKeywords(const QString &text) const;
    void setAiState(int state, const QString &modelName = QString());
    QString buildUserFilterContextHtml() const;
    int aiAvailabilityBackoffMs() const;
    QList<int> liveSearch(const QString &query) const;
    void applyUserFilterHighlights();
    void updateDomainStatus();

    QString errorText;
    DltChat::Form *form;
    QDltFile *dltFile;
    QTableView *mainTableView;
    QDltMessageDecoder *messageDecoder;
    QColor highlightColor;

    QVector<DltAnalyzerInterface::LogEntry> entries;
    QHash<int, int> indexToPos;
    mutable QMutex entriesMutex;

    QHash<QString, QSet<int>> invertedIndex;
    QSet<QString> indexStopwords;

    QHash<int, int> filterRowMap;
    mutable bool filterRowMapDirty = false;

    UserFilterManager *m_userFilterManager;
    QHash<int, QColor> m_highlightMap;
    static constexpr int kMaxDisplayResults = 1000;

    DltAnalyzerInterface *m_analyzer;
    DltRuleBasedAnalyzer *m_ruleBasedAnalyzer;
    DltLlmAnalyzerInterface *m_llmAnalyzer;
    QString m_currentAnalyzerType;

    // Contextual AI support
    ContextualExtractor m_contextualExtractor;
    ConversationManager m_conversationManager;
    FibexEnricher m_fibexEnricher;
    QList<int> m_lastSelectedIndices;

    DltBulkAnalyzer *m_bulkAnalyzer;
    bool m_bulkAnalysisEnabled;
    bool m_bulkAnalysisInProgress;
    void startBulkAnalysis();
    QVector<DltAnalyzerInterface::LogEntry> m_lastAiContext;

    int m_aiState = 0;
    QString m_aiModelName;
    QElapsedTimer m_aiAvailabilityTimer;
    static constexpr int AI_AVAILABILITY_TTL_MS = 30000;

    mutable QMutex m_llmMutex;
    mutable QMutex m_aiCacheMutex;
    bool m_llmRequestInProgress = false;
    QElapsedTimer m_llmRequestTimer;
    int m_aiAvailabilityRetryCount = 0;
    QHash<QString, DltAnalyzerInterface::QueryResult> m_aiResponseCache;
};

#endif
