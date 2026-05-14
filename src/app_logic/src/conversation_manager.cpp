#include "dltchat/conversation_manager.h"

namespace dltchat {

void ConversationManager::addTurn(const QString &role, const QString &message)
{
    if (m_history.size() >= MAX_HISTORY)
        m_history.removeFirst();

    ConversationTurn turn;
    turn.role = role;
    turn.message = message;
    turn.timestamp = QDateTime::currentDateTime();
    m_history.append(turn);
}

QString ConversationManager::formatHistory(int maxTurns) const
{
    if (m_history.isEmpty()) return QString();

    QStringList lines;
    int start = qMax(0, m_history.size() - maxTurns);

    lines << "Conversation history:";
    for (int i = start; i < m_history.size(); ++i)
    {
        const auto &turn = m_history[i];
        lines << QString("[%1] %2: %3")
            .arg(turn.timestamp.toString("HH:mm:ss"))
            .arg(turn.role)
            .arg(turn.message.left(200));
    }

    return lines.join("\n");
}

void ConversationManager::clear()
{
    m_history.clear();
}

bool ConversationManager::isEmpty() const
{
    return m_history.isEmpty();
}

int ConversationManager::turnCount() const
{
    return m_history.size();
}

} // namespace dltchat
