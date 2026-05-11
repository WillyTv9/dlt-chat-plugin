#ifndef DLTCHATPLUGIN_H
#define DLTCHATPLUGIN_H

#include <QObject>
#include <QColor>
#include <QHash>
#include <QMutex>
#include <QTableView>

#include "plugininterface.h"
#include "chatform.h"

#include "dltexport.h"
#include "dltanalyzerinterface.h"
#include "dltllmanalyzerinterface.h"
#include "qdltmessagedecoder.h"
#include "qdltfile.h"

#define DLT_CHAT_PLUGIN_VERSION "0.2.1"

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

    /* QDLTPluginInterface */
    QString name();
    QString pluginVersion();
    QString pluginInterfaceVersion();
    QString description();
    QString error();
    bool loadConfig(QString filename);
    bool saveConfig(QString filename);
    QStringList infoConfig();

    /* QDltPluginViewerInterface */
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

    /* QDltPluginControlInterface */
    bool initControl(QDltControl *control);
    bool initConnections(QStringList list);
    bool controlMsg(int index, QDltMsg &msg);
    bool stateChanged(int index, QDltConnection::QDltConnectionState connectionState, QString hostname);
    bool autoscrollStateChanged(bool enabled);
    void initMessageDecoder(QDltMessageDecoder* pMessageDecoder);
    void initMainTableView(QTableView* pTableView);
    void configurationChanged();

    /* Analyzer Management */
    void setAnalyzerType(const QString &type);
    QString currentAnalyzerType() const;
    void configureLlmAnalyzer(const QString &endpoint, const QString &apiKey, const QString &model);

signals:
    void statusChanged(const QString &text);

private slots:
    void onQuerySubmitted(const QString &query);
    void onIndexActivated(int index);
    void onClearHighlightsRequested();
    void onExportRequested(const QString &filePath, const QList<int> &indices, const QStringList &snippets, const QString &query);
    void onExportAllRequested(const QString &filePath);

private:
    void clearData();
    void ingestMessage(int index, QDltMsg &msg);
    DltAnalyzerInterface::QueryResult analyzeQueryInternal(const QString &query);
    void highlightIndices(const QList<int> &indices);
    int findRowForIndex(int index) const;
    void updateStatus(const QString &text);
    void setupDefaultAnalyzer();

    QString errorText;
    DltChat::Form *form;
    QDltFile *dltFile;
    QTableView *mainTableView;
    QDltMessageDecoder *messageDecoder;
    QColor highlightColor;

    QVector<DltAnalyzerInterface::LogEntry> entries;
    QHash<int, int> indexToPos;
    QMutex entriesMutex;

    DltAnalyzerInterface *m_analyzer;
    DltRuleBasedAnalyzer *m_ruleBasedAnalyzer;
    DltLlmAnalyzerInterface *m_llmAnalyzer;
    QString m_currentAnalyzerType;
};

#endif // DLTCHATPLUGIN_H
