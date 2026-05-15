#ifndef DLTCHAT_PLUGIN_PRIVATE_H
#define DLTCHAT_PLUGIN_PRIVATE_H

#include "plugin_entry.h"

struct DltChatPluginPrivate
{
    QString errorText;
    DltChat::Form *form = nullptr;
    QDltFile *dltFile = nullptr;
    QTableView *mainTableView = nullptr;
    QDltMessageDecoder *messageDecoder = nullptr;
    QColor highlightColor{255, 230, 128};

    QHash<int, int> filterRowMap;
    mutable bool filterRowMapDirty = false;
    static constexpr int kMaxDisplayResults = 10000000;

    dltchat::DltAnalyzerInterface *m_analyzer = nullptr;
    dltchat::DltRuleBasedAnalyzer *m_ruleBasedAnalyzer;
    dltchat::DltLlmAnalyzerInterface *m_llmAnalyzer;
    QString m_currentAnalyzerType{"rule-based"};

    bool m_bulkAnalysisEnabled = false;
    bool m_bulkAnalysisInProgress = false;

    int m_aiState = 0;
    QString m_aiModelName;
    QElapsedTimer m_aiAvailabilityTimer;
    static constexpr int AI_AVAILABILITY_TTL_MS = 30000;

    mutable QMutex m_llmMutex;
    bool m_llmRequestInProgress = false;
    QElapsedTimer m_llmRequestTimer;
    int m_aiAvailabilityRetryCount = 0;
};

#endif

