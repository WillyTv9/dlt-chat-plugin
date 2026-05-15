#include "test_conversationmanager.h"
#include "dltchat/conversation_manager.h"
#include <QTest>

using namespace dltchat;

void TestConversationManager::testEmptyHistory()
{
    ConversationManager mgr;
    QVERIFY(mgr.isEmpty());
    QCOMPARE(mgr.turnCount(), 0);
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
    QVERIFY(history.contains("Conversation history:"));
    QVERIFY(history.contains("user: Show me errors"));
    QVERIFY(history.contains("assistant: Found 3 errors"));
}

void TestConversationManager::testMaxTurns()
{
    ConversationManager mgr;
    // Add 55 turns (max is 50)
    for (int i = 0; i < 55; ++i) {
        mgr.addTurn("user", QString("query %1").arg(i));
        mgr.addTurn("assistant", QString("response %1").arg(i));
    }
    // Should have capped at 50
    QCOMPARE(mgr.turnCount(), 50);
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
