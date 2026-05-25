#include "dltchat/hierarchical_summary_store.h"

#include <QHashIterator>
#include <QMap>
#include <QMutexLocker>
#include <algorithm>

namespace dltchat {

void HierarchicalSummaryStore::setStatistics(const LogStatistics &stats)
{
    QMutexLocker lk(&m_mutex);
    m_stats = stats;
    m_ready = true;
}

LogStatistics HierarchicalSummaryStore::statistics() const
{
    QMutexLocker lk(&m_mutex);
    return m_stats;
}

void HierarchicalSummaryStore::setBlocks(const QVector<BlockSummary> &blocks)
{
    QMutexLocker lk(&m_mutex);
    m_blocks = blocks;
}

QVector<BlockSummary> HierarchicalSummaryStore::blocks() const
{
    QMutexLocker lk(&m_mutex);
    return m_blocks;
}

void HierarchicalSummaryStore::setEcuSummaries(const QHash<QString, EcuSummary> &ecuMap)
{
    QMutexLocker lk(&m_mutex);
    m_ecus = ecuMap;
}

QHash<QString, EcuSummary> HierarchicalSummaryStore::ecuSummaries() const
{
    QMutexLocker lk(&m_mutex);
    return m_ecus;
}

QVector<TimeWindowSummary> HierarchicalSummaryStore::byTimeWindow(qint64 windowMs) const
{
    QMutexLocker lk(&m_mutex);
    if (windowMs <= 0 || m_blocks.isEmpty())
        return {};

    QMap<qint64, TimeWindowSummary> buckets;
    for (const BlockSummary &b : m_blocks) {
        if (b.startMs == 0 && b.endMs == 0)
            continue;
        qint64 key = (b.startMs / windowMs) * windowMs;
        TimeWindowSummary &w = buckets[key];
        if (w.entryCount == 0) {
            w.startMs = key;
            w.endMs = key + windowMs;
        }
        w.entryCount += (b.lastIdx - b.firstIdx + 1);
        if (!w.synthesis.isEmpty())
            w.synthesis += "; ";
        w.synthesis += b.summary.left(80);
    }

    QVector<TimeWindowSummary> out;
    out.reserve(buckets.size());
    for (auto it = buckets.constBegin(); it != buckets.constEnd(); ++it)
        out.append(it.value());
    return out;
}

static QString formatHashTop(const QHash<QString, int> &h, int topN)
{
    QVector<QPair<QString, int>> v;
    v.reserve(h.size());
    for (auto it = h.constBegin(); it != h.constEnd(); ++it)
        v.append({ it.key(), it.value() });
    std::sort(v.begin(), v.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    QStringList parts;
    const int n = std::min(topN, v.size());
    parts.reserve(n);
    for (int i = 0; i < n; ++i)
        parts << QString("%1=%2").arg(v[i].first).arg(v[i].second);
    return parts.join(", ");
}

QString HierarchicalSummaryStore::compactDigest(int maxChars) const
{
    QMutexLocker lk(&m_mutex);
    if (!m_ready || maxChars <= 0)
        return {};

    QStringList out;
    out << QString("[STATS] entries=%1 ecus=%2 timespan_ms=%3")
               .arg(m_stats.totalEntries)
               .arg(m_stats.byEcu.size())
               .arg(m_stats.lastTimestampMs - m_stats.firstTimestampMs);

    if (!m_stats.byLevel.isEmpty())
        out << "[LEVELS] " + formatHashTop(m_stats.byLevel, 8);
    if (!m_stats.byEcu.isEmpty())
        out << "[ECUS] " + formatHashTop(m_stats.byEcu, 10);
    if (!m_stats.byApid.isEmpty())
        out << "[APIDS] " + formatHashTop(m_stats.byApid, 10);
    if (!m_stats.byCategory.isEmpty())
        out << "[CATEGORIES] " + formatHashTop(m_stats.byCategory, 10);
    if (!m_stats.byDomain.isEmpty())
        out << "[DOMAINS] " + formatHashTop(m_stats.byDomain, 6);

    int used = 0;
    for (const QString &line : out)
        used += line.size() + 1;

    const int remaining = maxChars - used;
    if (remaining > 200 && !m_blocks.isEmpty()) {
        out << "[BLOCKS]";
        int budgetLeft = remaining - 16;
        for (const BlockSummary &b : m_blocks) {
            QString line = QString("  #%1-%2: %3")
                               .arg(b.firstIdx)
                               .arg(b.lastIdx)
                               .arg(b.summary.left(120));
            if (!b.topApids.isEmpty())
                line += " | apids=" + b.topApids.mid(0, 3).join(",");
            if (line.size() + 1 > budgetLeft)
                break;
            out << line;
            budgetLeft -= line.size() + 1;
        }
    }

    QString joined = out.join('\n');
    if (joined.size() > maxChars)
        joined = joined.left(maxChars - 3) + "...";
    return joined;
}

void HierarchicalSummaryStore::clear()
{
    QMutexLocker lk(&m_mutex);
    m_stats = {};
    m_blocks.clear();
    m_ecus.clear();
    m_ready = false;
}

bool HierarchicalSummaryStore::isReady() const
{
    QMutexLocker lk(&m_mutex);
    return m_ready;
}

} // namespace dltchat
