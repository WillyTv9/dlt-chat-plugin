#include "dltchat/log_index.h"

#include <algorithm>

namespace dltchat {

void LogIndex::add(const QString &keyword, int entryIndex)
{
    m_index[keyword].insert(entryIndex);
}

QSet<int> LogIndex::lookup(const QString &keyword) const
{
    auto it = m_index.find(keyword);
    if (it != m_index.end())
        return it.value();
    return {};
}

QSet<int> LogIndex::intersect(const QStringList &keywords) const
{
    QSet<int> result;
    bool first = true;

    for (const QString &kw : keywords)
    {
        QSet<int> set = lookup(kw);
        if (set.isEmpty())
            return {};

        if (first)
        {
            result = set;
            first = false;
        }
        else
        {
            result.intersect(set);
        }

        if (result.isEmpty())
            return {};
    }

    return result;
}

bool LogIndex::contains(const QString &keyword) const
{
    return m_index.contains(keyword);
}

void LogIndex::clear()
{
    m_index.clear();
}

int LogIndex::size() const
{
    return m_index.size();
}

QSet<QString> LogIndex::defaultStopwords()
{
    return {
        "the", "and", "this", "that", "what", "which", "with", "from", "have", "been",
        "show", "mostra", "elenca", "tutti", "tutte", "all", "why", "perche", "causa",
        "motivo", "summary", "summarize", "riassumi", "sintesi", "log", "logs",
        "messaggi", "messaggio", "indice", "index", "riga", "line", "timestamp", "time",
        "tempo", "error", "errors", "errore", "errori", "fatal", "fatale", "warn",
        "warning", "avviso", "info", "debug", "verbose", "context", "before", "after",
        "can", "not", "are", "was", "for", "will", "has", "had", "but", "its", "also"
    };
}

bool LogIndex::isStopword(const QString &word) const
{
    return m_stopwords.contains(word);
}

} // namespace dltchat
