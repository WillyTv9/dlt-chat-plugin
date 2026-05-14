#ifndef DLTCHAT_CONVERSATION_MANAGER_H
#define DLTCHAT_CONVERSATION_MANAGER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>

namespace dltchat {

struct ConversationTurn {
    QString role;
    QString message;
    QDateTime timestamp;
};

class ConversationManager
{
public:
    ConversationManager() = default;

    void addTurn(const QString &role, const QString &message);
    QString formatHistory(int maxTurns = 5) const;
    void clear();
    bool isEmpty() const;
    int turnCount() const;

private:
    QVector<ConversationTurn> m_history;
    static constexpr int MAX_HISTORY = 50;
};

} // namespace dltchat

#endif
