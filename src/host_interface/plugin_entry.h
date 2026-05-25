#ifndef DLTCHAT_PLUGIN_ENTRY_H
#define DLTCHAT_PLUGIN_ENTRY_H

#include <QObject>
#include <QColor>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QTableView>
#include <QElapsedTimer>

#include "plugininterface.h"
#include "chatform.h"

#include "dltchat/export_engine.h"
#include "dltchat/analyzer_interface.h"
#include "dltchat/llm_analyzer_interface.h"
#include "dltchat/automotive_log_parser.h"
#include "dltchat/contextual_extractor.h"
#include "dltchat/conversation_manager.h"
#include "dltchat/fibex_enricher.h"
#include "dltchat/user_filter_manager.h"
#include "dltchat/bulk_analyzer.h"
#include "dltchat/hierarchical_summary_store.h"
#include "dltchat/log_ingestion_pipeline.h"
#include "dltchat/ai_query_pipeline.h"
#include "dltchat/map_reduce_analyzer.h"
#include "dltchat/ai_error_reporter.h"
#include "native_filter_catalog.h"
#include "highlight_delegate.h"
#include "qdltmessagedecoder.h"
#include "qdltfile.h"



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
    void onQuickActionQuery(const QString &query);
    void onNativeFilterTriggered(const QString &filterName);
    void onNativeFilterGroupTriggered(const QString &groupId);
    void onAiQuerySubmitted(const QString &query);
    void onConfigureAiClicked();
    void onIndexActivated(int index);
    void onClearHighlightsRequested();
    void onExportRequested(const QString &filePath, const QList<int> &indices, const QStringList &snippets, const QString &query);
    void onExportAllRequested(const QString &filePath);
    void onLlmResultReady(const dltchat::DltAnalyzerInterface::QueryResult &result, const QString &originalQuery);
    void onUserFilterLoadRequested(const QString &path);
    void onBulkProgress(double progress, int processed, int total);
    void onBulkFinished(bool success);
    void onBulkError(const QString &error);
    void onAiHealthCheck();

    void onAiPipelinePrepared(const dltchat::AiQueryPipeline::Result &prep);
    void onAiPipelineFailed(const QString &stage, const QString &reason);
    void onMapReduceShardCompleted(int shardIdx, int total, qint64 elapsedMs);
    void onMapReduceReduceReady(const dltchat::DltAnalyzerInterface::QueryResult &result,
                                const QString &originalQuery);
    void onMapReduceFailed(const QString &stage, const QString &reason);
    void onIngestionReady();
    void onIngestionStageProgress(const QString &stage, int pct);
    void onIngestionFailed(const QString &stage, const QString &reason);

private:
    void clearData();
    void ingestMessage(int index, QDltMsg &msg);
    void rebuildFilterRowMap() const;
    void highlightIndices(const QList<int> &indices);
    void highlightIndicesColored(const QList<int> &indices, const QColor &color);
    void loadNativeFilterCatalog();
    void populateNativeFilterMenus();
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
    void startBulkAnalysis();
    void applyFiltersToHost(const QList<DltChat::NativeFilterAction> &actions);
    void clearPluginFiltersFromHost();

    void onLiveAiRefresh();

    static constexpr const char *kPluginFilterPrefix = "[chat]";

    QString errorText;
    DltChat::Form *form;
    QDltFile *dltFile;
    QTableView *mainTableView;
    QDltMessageDecoder *messageDecoder;
    QColor highlightColor;

    QVector<dltchat::DltAnalyzerInterface::LogEntry> entries;
    QHash<int, int> indexToPos;
    mutable QMutex entriesMutex;

    QHash<QString, QSet<int>> invertedIndex;
    QSet<QString> indexStopwords;

    mutable QHash<int, int> filterRowMap;
    mutable bool filterRowMapDirty = false;

    dltchat::UserFilterManager *m_userFilterManager;
    QHash<int, QString> m_highlightMap;

    // Native .dlp-driven Quick Actions.
    DltChat::NativeFilterCatalog m_nativeFilterCatalog;
    DltChat::HighlightDelegate  *m_highlightDelegate = nullptr;
    QString m_dlpFilterPath;   //!< optional override from dlt_chat_plugin.ini ([Filters] dlpPath)

    dltchat::DltAnalyzerInterface *m_analyzer;
    dltchat::DltRuleBasedAnalyzer *m_ruleBasedAnalyzer;
    dltchat::DltLlmAnalyzerInterface *m_llmAnalyzer;
    QString m_currentAnalyzerType;

    dltchat::ContextualExtractor m_contextualExtractor;
    dltchat::ConversationManager m_conversationManager;
    dltchat::FibexEnricher m_fibexEnricher;
    QList<int> m_lastSelectedIndices;

    dltchat::DltBulkAnalyzer *m_bulkAnalyzer;
    bool m_bulkAnalysisEnabled;
    bool m_bulkAnalysisInProgress;
    QVector<dltchat::DltAnalyzerInterface::LogEntry> m_lastAiContext;

    dltchat::HierarchicalSummaryStore m_hierStore;
    dltchat::LogIngestionPipeline *m_ingestionPipeline = nullptr;
    dltchat::AiQueryPipeline *m_aiQueryPipeline = nullptr;
    dltchat::MapReduceAnalyzer *m_mapReduceAnalyzer = nullptr;
    QString m_pendingAiQuery;
    bool m_pendingAiIsMapReduce = false;
    int m_ingestBlockSize = 5000;

    // Live-mode fields
    bool m_liveMode = false;
    int m_liveAiRefreshSec = 30;
    int m_lastIngestionEntryCount = 0;
    QTimer *m_liveAiRefreshTimer = nullptr;

    int m_aiState = 0;
    QString m_aiModelName;
    QElapsedTimer m_aiAvailabilityTimer;
    static constexpr int AI_AVAILABILITY_TTL_MS = 30000;

    mutable QMutex m_llmMutex;
    mutable QMutex m_aiCacheMutex;
    bool m_llmRequestInProgress = false;
    QElapsedTimer m_llmRequestTimer;
    int m_aiAvailabilityRetryCount = 0;
    QHash<QString, dltchat::DltAnalyzerInterface::QueryResult> m_aiResponseCache;
    QString m_copilotOAuthToken;
    QString m_copilotClientId;
    QTimer *m_aiHealthTimer = nullptr;
};

#endif

