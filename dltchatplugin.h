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
#include "qdltmessagedecoder.h"
#include "qdltfile.h"

#define DLT_CHAT_PLUGIN_VERSION "0.3.0"

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

    QString name();
    QString pluginVersion();
    QString pluginInterfaceVersion();
    QString description();
    QString error();
    bool loadConfig(QString filename);
    bool saveConfig(QString filename);
    QStringList infoConfig();

    QWidget* initViewer();
    void initFileStart(QDltFile *file);
    void initFileFinish();
    void initMsg(int index, QDltMsg &msg);
    void initMsgDecoded(int index, QDltMsg &msg);
    void updateFileStart();
    void updateMsg(int index, QDltMsg &msg);
    void updateMsgDecoded(int index, QDltMsg &msg);
    void updateFileFinish();
    void selectedIdxMsg(int index, QDltMsg &msg);
    void selectedIdxMsgDecoded(int index, QDltMsg &msg);

    bool initControl(QDltControl *control);
    bool initConnections(QStringList list);
    bool controlMsg(int index, QDltMsg &msg);
    bool stateChanged(int index, QDltConnection::QDltConnectionState connectionState, QString hostname);
    bool autoscrollStateChanged(bool enabled);
    void initMessageDecoder(QDltMessageDecoder* pMessageDecoder);
    void initMainTableView(QTableView* pTableView);
    void configurationChanged();

    void setAnalyzerType(const QString &type);
    QString currentAnalyzerType() const;
    void configureLlmAnalyzer(const QString &endpoint, const QString &apiKey, const QString &model);

    enum AiState { AiUnconfigured = 0, AiConfiguredOffline = 1, AiConnected = 2 };

signals:
    void statusChanged(const QString &text);

private slots:
    void onQuerySubmitted(const QString &query);
    void onAiQuerySubmitted(const QString &query);
    void onConfigureAiClicked();
    void onIndexActivated(int index);
    void onClearHighlightsRequested();
    void onExportRequested(const QString &filePath, const QList<int> &indices, const QStringList &snippets, const QString &query);
    void onExportAllRequested(const QString &filePath);
    void onLlmResultReady(const DltAnalyzerInterface::QueryResult &result, const QString &originalQuery);
    void onAiAvailabilityChanged(int state, const QString &modelName);

private:
    void clearData();
    void ingestMessage(int index, QDltMsg &msg);
    void analyzeQueryFast(const QString &query, bool preferAi);
    void rebuildFilterRowMap();
    void highlightIndices(const QList<int> &indices);
    int findRowForIndex(int index) const;
    void updateStatus(const QString &text);
    void setupDefaultAnalyzer();
    void applyConfigToForm();
    void checkAiAvailabilityAsync();
    QStringList extractKeywords(const QString &text) const;
    void setAiState(int state, const QString &modelName = QString());
    static bool isDarkMode(const QWidget *w);

    QString errorText;
    DltChat::Form *form;
    QDltFile *dltFile;
    QTableView *mainTableView;
    QDltMessageDecoder *messageDecoder;
    QColor highlightColor;

    QVector<DltAnalyzerInterface::LogEntry> entries;
    QHash<int, int> indexToPos;
    QMutex entriesMutex;

    QHash<QString, QSet<int>> invertedIndex;
    QSet<QString> indexStopwords;

    QHash<int, int> filterRowMap;
    bool filterRowMapDirty = false;

    DltAnalyzerInterface *m_analyzer;
    DltRuleBasedAnalyzer *m_ruleBasedAnalyzer;
    DltLlmAnalyzerInterface *m_llmAnalyzer;
    QString m_currentAnalyzerType;

    int m_aiState = 0;
    QString m_aiModelName;
    QElapsedTimer m_aiAvailabilityTimer;
    static constexpr int AI_AVAILABILITY_TTL_MS = 30000;

    mutable QMutex m_llmMutex;
    bool m_llmRequestInProgress = false;
    QString m_pendingAiQuery;
};

#endif
