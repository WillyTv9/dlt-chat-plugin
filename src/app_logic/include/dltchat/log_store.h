#ifndef DLTCHAT_LOG_STORE_H
#define DLTCHAT_LOG_STORE_H

#include "analyzer_interface.h"

#include <QHash>
#include <QMutex>
#include <QStringList>
#include <QVector>
#include <functional>

namespace dltchat {

class LogStore
{
public:
    using LogEntry = DltAnalyzerInterface::LogEntry;

    void append(const LogEntry &entry);
    LogEntry at(int pos) const;
    int size() const;
    bool isEmpty() const;
    QVector<LogEntry> copyAll() const;
    void clear();
    void squeeze();

    QHash<int, int> indexToPos() const;
    void setIndexToPos(int index, int pos);
    bool hasIndex(int index) const;

    QHash<QString, QSet<int>> invertedIndex() const;
    void addToIndex(const QString &keyword, int entryIndex);
    QStringList extractKeywords(const QString &text, const QSet<QString> &stopwords = {}) const;

    QSet<QString> stopwords() const { return m_stopwords; }
    void setStopwords(const QSet<QString> &sw) { m_stopwords = sw; }

    static constexpr int kMaxEntries = 500000;

private:
    QVector<LogEntry> m_entries;
    QHash<int, int> m_indexToPos;
    QHash<QString, QSet<int>> m_invertedIndex;
    QSet<QString> m_stopwords;
    mutable QMutex m_mutex;
};

} // namespace dltchat

#endif
