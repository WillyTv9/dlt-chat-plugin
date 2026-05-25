#ifndef DLTCHAT_AI_ERROR_REPORTER_H
#define DLTCHAT_AI_ERROR_REPORTER_H

#include <QString>

namespace dltchat {

class AiErrorReporter
{
public:
    struct Context {
        QString provider;      // e.g. "copilot/gpt-4o"
        QString model;
        int budgetChars = 0;
        int entryCount = 0;
        int shardIdx = -1;     // -1 if not applicable
        int shardTotal = -1;
        QString diagnostic;    // free-form trace (from BudgetPlan, EnhancedRetriever, etc.)
        QString hint;          // optional explicit hint; if empty, helper derives one
    };

    static QString formatHtml(const QString &stage, const QString &cause, const Context &ctx);
    static QString deriveHint(const QString &stage, const QString &cause);
};

} // namespace dltchat

#endif // DLTCHAT_AI_ERROR_REPORTER_H
