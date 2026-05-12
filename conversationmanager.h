#ifndef CONVERSATIONMANAGER_H
#define CONVERSATIONMANAGER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>

class ConversationManager
{
public:
    struct Turn
    {
        QString role;    // "user" or "assistant"
        QString content;
        QDateTime timestamp;
    };

    ConversationManager();

    void addTurn(const QString &role, const QString &content);
    QString formatHistory(int maxExchanges = 3) const;
    void clear();

    QString lastUserQuery() const;
    QString lastResponse() const;
    int turnCount() const { return m_turns.size(); }
    bool isEmpty() const { return m_turns.isEmpty(); }

    QVector<Turn> recentTurns(int maxExchanges) const;

private:
    QVector<Turn> m_turns;
    int m_maxTurns = 20;
};

#endif
