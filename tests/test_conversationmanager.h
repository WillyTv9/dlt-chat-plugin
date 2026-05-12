#ifndef TEST_CONVERSATIONMANAGER_H
#define TEST_CONVERSATIONMANAGER_H

#include <QObject>

class TestConversationManager : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyHistory();
    void testAddTurn();
    void testFormatHistory();
    void testLastUserQuery();
    void testLastResponse();
    void testMaxTurns();
    void testClear();
};

#endif
