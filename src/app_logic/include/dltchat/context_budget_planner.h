#ifndef DLTCHAT_CONTEXT_BUDGET_PLANNER_H
#define DLTCHAT_CONTEXT_BUDGET_PLANNER_H

#include "model_profile_registry.h"

#include <QString>

namespace dltchat {

/**
 * Output of ContextBudgetPlanner::plan(). All char fields are budgets in
 * CHARS (not tokens) — multiply tokens by tokensToCharsRatio (≈3.5 for
 * IT/EN automotive payloads). This keeps the rest of the code free of
 * tokenizer assumptions.
 */
struct BudgetPlan
{
    int hierChars = 0;       // for HierarchicalSummaryStore::compactDigest
    int rawChars = 0;        // total chars budget for raw log entries
    int slackChars = 0;      // safety margin not consumed by either
    int maxEntriesHint = 0;  // rough cap on number of raw entries to pass
    bool isGlobalQuery = false;
    bool deepRequested = false; // user prefixed query with "deep:"
    ModelProfile profile;
    QString diagnostic; // human-readable trace for UI error reports
};

class ContextBudgetPlanner
{
public:
    static constexpr double kTokensToChars = 3.5;
    static constexpr int kSystemPromptTokensEstimate = 800;
    static constexpr int kAvgEntryChars = 150;

    /**
     * Compute the budget for a single AI query.
     *
     * @param query                  user query (used for global-vs-specific detection)
     * @param provider               provider key (e.g. "copilot", "ollama")
     * @param model                  model name (e.g. "gpt-4o")
     * @param conversationHistoryChars chars already consumed by recent turns
     */
    static BudgetPlan plan(const QString &query,
                           const QString &provider,
                           const QString &model,
                           int conversationHistoryChars = 0);

    /**
     * True when the query asks for an aggregate / panoramic view that
     * justifies routing through MapReduceAnalyzer rather than the
     * single-shot path.
     */
    static bool isGlobalQuery(const QString &query);

    /**
     * Detect the explicit "deep:" prefix (case-insensitive) and return
     * the stripped query via `stripped`. Lets a power-user force the
     * map-reduce path without any UI element.
     */
    static bool hasDeepPrefix(const QString &query, QString *stripped = nullptr);
};

} // namespace dltchat

#endif
