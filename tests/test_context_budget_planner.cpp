#include "test_context_budget_planner.h"

#include "dltchat/context_budget_planner.h"

#include <QTest>

using namespace dltchat;

void TestContextBudgetPlanner::init()
{
}

void TestContextBudgetPlanner::cleanup()
{
}

void TestContextBudgetPlanner::testGlobalQueryDetection()
{
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("riassumi gli errori"));
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("elenca tutti i messaggi"));
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("quanti errori ci sono"));
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("dammi una panoramica"));

    QVERIFY(!ContextBudgetPlanner::isGlobalQuery("perché bluetooth non funziona"));
    QVERIFY(!ContextBudgetPlanner::isGlobalQuery("errore connessione USB"));
    QVERIFY(!ContextBudgetPlanner::isGlobalQuery(""));
}

void TestContextBudgetPlanner::testEnglishGlobalDetection()
{
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("summarize the session"));
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("how many errors"));
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("give me an overall view"));
    QVERIFY(ContextBudgetPlanner::isGlobalQuery("list all warnings"));

    QVERIFY(!ContextBudgetPlanner::isGlobalQuery("why did the bluetooth fail"));
    QVERIFY(!ContextBudgetPlanner::isGlobalQuery("show stacktrace for crash"));
}

void TestContextBudgetPlanner::testDeepPrefix()
{
    QString stripped;
    QVERIFY(ContextBudgetPlanner::hasDeepPrefix("deep: riassumi", &stripped));
    QCOMPARE(stripped, QString("riassumi"));

    QVERIFY(ContextBudgetPlanner::hasDeepPrefix("DEEP:analizza tutto", &stripped));
    QCOMPARE(stripped, QString("analizza tutto"));

    QVERIFY(!ContextBudgetPlanner::hasDeepPrefix("riassumi", &stripped));
    QVERIFY(!ContextBudgetPlanner::hasDeepPrefix("error message", nullptr));

    // The plan() entry-point honours the prefix and flags both fields.
    BudgetPlan plan = ContextBudgetPlanner::plan("deep: riassumi", "copilot", "gpt-4o");
    QVERIFY(plan.deepRequested);
    QVERIFY(plan.isGlobalQuery);
}

void TestContextBudgetPlanner::testPlanForCopilot()
{
    BudgetPlan plan = ContextBudgetPlanner::plan(
        "perché bluetooth non si connette", "copilot", "gpt-4o");

    QCOMPARE(plan.profile.maxContextTokens, 128000);
    QVERIFY(!plan.isGlobalQuery);
    QVERIFY(!plan.deepRequested);
    // Specific query => more raw budget than hierarchical digest.
    QVERIFY2(plan.rawChars > plan.hierChars,
             qPrintable(QString("raw=%1 hier=%2").arg(plan.rawChars).arg(plan.hierChars)));
    QVERIFY(plan.maxEntriesHint >= 10);
    QVERIFY(!plan.diagnostic.isEmpty());
}

void TestContextBudgetPlanner::testPlanForGlobalQuery()
{
    BudgetPlan plan = ContextBudgetPlanner::plan(
        "riassumi gli errori di sessione", "copilot", "gpt-4o");

    QVERIFY(plan.isGlobalQuery);
    // Global query => hierarchical digest gets the lion's share.
    QVERIFY2(plan.hierChars > plan.rawChars,
             qPrintable(QString("hier=%1 raw=%2").arg(plan.hierChars).arg(plan.rawChars)));
}

void TestContextBudgetPlanner::testPlanForOllama()
{
    BudgetPlan ollama = ContextBudgetPlanner::plan("error analysis", "ollama", "llama3");
    BudgetPlan copilot = ContextBudgetPlanner::plan("error analysis", "copilot", "gpt-4o");

    // Ollama llama3 has a much smaller window (8192) vs Copilot gpt-4o (128000),
    // so its total char budget must be strictly smaller.
    const int ollamaTotal = ollama.hierChars + ollama.rawChars + ollama.slackChars;
    const int copilotTotal = copilot.hierChars + copilot.rawChars + copilot.slackChars;
    QVERIFY2(ollamaTotal < copilotTotal,
             qPrintable(QString("ollama=%1 copilot=%2").arg(ollamaTotal).arg(copilotTotal)));
    QCOMPARE(ollama.profile.maxContextTokens, 8192);
}
