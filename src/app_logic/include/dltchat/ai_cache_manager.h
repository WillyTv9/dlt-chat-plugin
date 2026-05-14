#ifndef DLTCHAT_AI_CACHE_MANAGER_H
#define DLTCHAT_AI_CACHE_MANAGER_H

#include "analyzer_interface.h"

#include <QHash>
#include <QMutex>
#include <QString>
#include <QVector>

namespace dltchat {

class AICacheManager
{
public:
    using LogEntry = DltAnalyzerInterface::LogEntry;
    using QueryResult = DltAnalyzerInterface::QueryResult;

    AICacheManager(int maxEntries = 10000);

    bool tryGet(const QString &cacheKey, QueryResult &outResult);
    void insert(const QString &cacheKey, const QueryResult &result);
    void clear();
    int size() const;

    static QString buildCacheKey(const QString &query, const QVector<LogEntry> &entries);

private:
    struct CacheEntry {
        QueryResult result;
        qint64 timestamp;
    };

    QHash<QString, CacheEntry> m_cache;
    QList<QString> m_accessOrder;
    int m_maxEntries;
    mutable QMutex m_mutex;
};

} // namespace dltchat

#endif
