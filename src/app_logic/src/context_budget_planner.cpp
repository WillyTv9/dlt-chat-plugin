#include "dltchat/context_budget_planner.h"

#include <QRegularExpression>
#include <algorithm>

namespace dltchat {

bool ContextBudgetPlanner::hasDeepPrefix(const QString &query, QString *stripped)
{
    static const QRegularExpression re(QStringLiteral("^\\s*deep\\s*:\\s*"),
                                       QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(query);
    if (!m.hasMatch())
        return false;
    if (stripped)
        *stripped = query.mid(m.capturedEnd()).trimmed();
    return true;
}

bool ContextBudgetPlanner::isGlobalQuery(const QString &query)
{
    // IT + EN trigger words that imply panoramic / aggregate analysis.
    static const QRegularExpression re(
        QStringLiteral("\\b("
                       "riassumi|riassunto|elenca|elenco|quali|quanti|quanto|"
                       "distribuzione|distribuisci|pattern|tutti|tutte|"
                       "totale|complessivo|panoramica|globale|"
                       "sintetizza|sintesi|panorama|"
                       "summarize|summary|list|how many|count|"
                       "distribution|overall|whole|entire|"
                       "patterns|trends|analy[sz]e the (log|session)"
                       ")\\b"),
        QRegularExpression::CaseInsensitiveOption);
    return re.match(query).hasMatch();
}

BudgetPlan ContextBudgetPlanner::plan(const QString &query,
                                      const QString &provider,
                                      const QString &model,
                                      int conversationHistoryChars)
{
    BudgetPlan p;
    QString effective = query;
    p.deepRequested = hasDeepPrefix(query, &effective);
    p.isGlobalQuery = p.deepRequested || isGlobalQuery(effective);

    p.profile = ModelProfileRegistry::profileFor(provider, model);

    const int budgetTokens =
        p.profile.maxContextTokens - p.profile.reservedForResponse - kSystemPromptTokensEstimate
        - static_cast<int>(conversationHistoryChars / kTokensToChars);

    const int safeBudgetTokens = std::max(budgetTokens, 1024);
    const int budgetChars = static_cast<int>(safeBudgetTokens * kTokensToChars);

    double hierFrac, rawFrac, slackFrac;
    if (p.isGlobalQuery) {
        hierFrac = 0.70;
        rawFrac = 0.20;
        slackFrac = 0.10;
    } else {
        hierFrac = 0.25;
        rawFrac = 0.65;
        slackFrac = 0.10;
    }

    p.hierChars = static_cast<int>(budgetChars * hierFrac);
    p.rawChars = static_cast<int>(budgetChars * rawFrac);
    p.slackChars = static_cast<int>(budgetChars * slackFrac);
    p.maxEntriesHint = std::max(10, p.rawChars / kAvgEntryChars);

    p.diagnostic = QStringLiteral(
        "provider=%1 model=%2 ctx=%3 reserved=%4 history=%5 budget_chars=%6 "
        "split(hier/raw/slack)=%7/%8/%9 maxEntriesHint=%10 isGlobal=%11 deep=%12 fallback=%13")
        .arg(p.profile.provider, p.profile.model)
        .arg(p.profile.maxContextTokens)
        .arg(p.profile.reservedForResponse)
        .arg(conversationHistoryChars)
        .arg(budgetChars)
        .arg(p.hierChars)
        .arg(p.rawChars)
        .arg(p.slackChars)
        .arg(p.maxEntriesHint)
        .arg(p.isGlobalQuery ? "yes" : "no")
        .arg(p.deepRequested ? "yes" : "no")
        .arg(p.profile.isFallback ? "yes" : "no");

    return p;
}

} // namespace dltchat
