#include "dltchat/ai_cache_manager.h"

#include <QCryptographicHash>
#include <QDateTime>

namespace dltchat {

AICacheManager::AICacheManager(int maxEntries)
    : m_maxEntries(maxEntries)
{
}

bool AICacheManager::tryGet(const QString &cacheKey, QueryResult &outResult)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_cache.find(cacheKey);
    if (it == m_cache.end())
        return false;

    m_accessOrder.removeAll(cacheKey);
    m_accessOrder.append(cacheKey);
    outResult = it.value().result;
    return true;
}

void AICacheManager::insert(const QString &cacheKey, const QueryResult &result)
{
    QMutexLocker lock(&m_mutex);

    if (m_cache.size() >= m_maxEntries)
    {
        int removeCount = m_accessOrder.size() / 2;
        for (int i = 0; i < removeCount && !m_accessOrder.isEmpty(); ++i)
        {
            m_cache.remove(m_accessOrder.first());
            m_accessOrder.removeFirst();
        }
    }

    m_accessOrder.removeAll(cacheKey);
    m_accessOrder.append(cacheKey);

    CacheEntry entry;
    entry.result = result;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_cache[cacheKey] = entry;
}

void AICacheManager::clear()
{
    QMutexLocker lock(&m_mutex);
    m_cache.clear();
    m_accessOrder.clear();
}

int AICacheManager::size() const
{
    QMutexLocker lock(&m_mutex);
    return m_cache.size();
}

QString AICacheManager::buildCacheKey(const QString &query, const QVector<LogEntry> &entries)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(query.toUtf8());
    int n = qMin(entries.size(), 20);
    for (int i = 0; i < n; ++i)
        hash.addData(QByteArray::number(entries[i].index));
    hash.addData(QByteArray::number(entries.size()));
    return hash.result().toHex();
}

} // namespace dltchat
