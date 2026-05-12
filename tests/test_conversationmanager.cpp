#include "test_conversationmanager.h"
#include "conversationmanager.h"
#include <QTest>

void TestConversationManager::testEmptyHistory()
{
    ConversationManager mgr;
    QVERIFY(mgr.isEmpty());
    QCOMPARE(mgr.turnCount(), 0);
    QVERIFY(mgr.lastUserQuery().isEmpty());
    QVERIFY(mgr.lastResponse().isEmpty());
    QVERIFY(mgr.formatHistory(3).isEmpty());
}

void TestConversationManager::testAddTurn()
{
    ConversationManager mgr;
    mgr.addTurn("user", "What caused the timeout?");
    QCOMPARE(mgr.turnCount(), 1);
    QVERIFY(!mgr.isEmpty());

    mgr.addTurn("assistant", "The timeout occurred in PWRM context.");
    QCOMPARE(mgr.turnCount(), 2);
}

void TestConversationManager::testFormatHistory()
{
    ConversationManager mgr;
    mgr.addTurn("user", "Show me errors");
    mgr.addTurn("assistant", "Found 3 errors");

    QString history = mgr.formatHistory(3);
    QVERIFY(history.contains("Previous conversation:"));
    QVERIFY(history.contains("User: Show me errors"));
    QVERIFY(history.contains("Assistant: Found 3 errors"));
}

void TestConversationManager::testLastUserQuery()
{
    ConversationManager mgr;
    mgr.addTurn("user", "first query");
    mgr.addTurn("assistant", "first response");
    mgr.addTurn("user", "second query");

    QCOMPARE(mgr.lastUserQuery(), "second query");
}

void TestConversationManager::testLastResponse()
{
    ConversationManager mgr;
    mgr.addTurn("user", "query");
    mgr.addTurn("assistant", "response one");
    mgr.addTurn("user", "follow-up");
    mgr.addTurn("assistant", "response two");

    QCOMPARE(mgr.lastResponse(), "response two");
}

void TestConversationManager::testMaxTurns()
{
    ConversationManager mgr;
    // Add 25 turns (max is 20)
    for (int i = 0; i < 25; ++i) {
        mgr.addTurn("user", QString("query %1").arg(i));
        mgr.addTurn("assistant", QString("response %1").arg(i));
    }
    // Should have capped at 20
    QCOMPARE(mgr.turnCount(), 20);
}

void TestConversationManager::testClear()
{
    ConversationManager mgr;
    mgr.addTurn("user", "query");
    mgr.addTurn("assistant", "response");
    QVERIFY(!mgr.isEmpty());

    mgr.clear();
    QVERIFY(mgr.isEmpty());
    QCOMPARE(mgr.turnCount(), 0);
}
