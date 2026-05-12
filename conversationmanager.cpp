#include "conversationmanager.h"

ConversationManager::ConversationManager()
{
}

void ConversationManager::addTurn(const QString &role, const QString &content)
{
    Turn t;
    t.role = role;
    t.content = content;
    t.timestamp = QDateTime::currentDateTime();

    m_turns.append(t);

    // Keep only the most recent turns
    while (m_turns.size() > m_maxTurns)
        m_turns.removeFirst();
}

QString ConversationManager::formatHistory(int maxExchanges) const
{
    if (m_turns.isEmpty())
        return QString();

    QVector<Turn> recent = recentTurns(maxExchanges);
    if (recent.isEmpty())
        return QString();

    QStringList lines;
    lines << "Previous conversation:";
    for (const auto &turn : recent) {
        QString roleLabel = (turn.role == "user") ? "User" : "Assistant";
        lines << QString("%1: %2").arg(roleLabel, turn.content);
    }

    return lines.join("\n");
}

void ConversationManager::clear()
{
    m_turns.clear();
}

QString ConversationManager::lastUserQuery() const
{
    for (int i = m_turns.size() - 1; i >= 0; --i) {
        if (m_turns[i].role == "user")
            return m_turns[i].content;
    }
    return QString();
}

QString ConversationManager::lastResponse() const
{
    for (int i = m_turns.size() - 1; i >= 0; --i) {
        if (m_turns[i].role == "assistant")
            return m_turns[i].content;
    }
    return QString();
}

QVector<ConversationManager::Turn> ConversationManager::recentTurns(int maxExchanges) const
{
    if (m_turns.isEmpty() || maxExchanges <= 0)
        return {};

    // Each exchange is 2 turns (user + assistant), so maxExchanges * 2 entries
    int count = qMin(m_turns.size(), maxExchanges * 2);
    return m_turns.mid(m_turns.size() - count);
}
