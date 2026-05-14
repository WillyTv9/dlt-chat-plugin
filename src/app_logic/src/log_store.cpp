#include "dltchat/log_store.h"

#include <QMutexLocker>
#include <QRegularExpression>

namespace dltchat {

void LogStore::append(const LogEntry &entry)
{
    QMutexLocker lock(&m_mutex);
    if (m_entries.size() >= kMaxEntries) return;
    m_entries.append(entry);
}

LogStore::LogEntry LogStore::at(int pos) const
{
    QMutexLocker lock(&m_mutex);
    if (pos >= 0 && pos < m_entries.size())
        return m_entries[pos];
    return LogEntry();
}

int LogStore::size() const
{
    QMutexLocker lock(&m_mutex);
    return m_entries.size();
}

bool LogStore::isEmpty() const
{
    QMutexLocker lock(&m_mutex);
    return m_entries.isEmpty();
}

QVector<LogStore::LogEntry> LogStore::copyAll() const
{
    QMutexLocker lock(&m_mutex);
    return m_entries;
}

void LogStore::clear()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
    m_indexToPos.clear();
    m_invertedIndex.clear();
}

void LogStore::squeeze()
{
    QMutexLocker lock(&m_mutex);
    m_entries.squeeze();
}

QHash<int, int> LogStore::indexToPos() const
{
    QMutexLocker lock(&m_mutex);
    return m_indexToPos;
}

void LogStore::setIndexToPos(int index, int pos)
{
    QMutexLocker lock(&m_mutex);
    m_indexToPos[index] = pos;
}

bool LogStore::hasIndex(int index) const
{
    QMutexLocker lock(&m_mutex);
    return m_indexToPos.contains(index);
}

QHash<QString, QSet<int>> LogStore::invertedIndex() const
{
    QMutexLocker lock(&m_mutex);
    return m_invertedIndex;
}

void LogStore::addToIndex(const QString &keyword, int entryIndex)
{
    QMutexLocker lock(&m_mutex);
    m_invertedIndex[keyword].insert(entryIndex);
}

QStringList LogStore::extractKeywords(const QString &text, const QSet<QString> &stopwords) const
{
    QSet<QString> sw = stopwords.isEmpty() ? m_stopwords : stopwords;
    QStringList tokens = text.toLower().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    QStringList kw;
    kw.reserve(tokens.size());
    for (const QString &t : tokens)
    {
        if (t.size() < 3) continue;
        if (sw.contains(t)) continue;
        if (t.at(0).isDigit()) continue;
        kw.append(t);
    }
    kw.removeDuplicates();
    return kw;
}

} // namespace dltchat
