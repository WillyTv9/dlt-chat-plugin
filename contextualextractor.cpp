#include "contextualextractor.h"

#include <QRegularExpression>
#include <QSet>
#include <algorithm>

static QStringList extractKeywordsSimple(const QString &text)
{
    if (text.trimmed().isEmpty()) return {};
    QStringList tokens = text.toLower().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    QSet<QString> stopwords = {
        "the", "and", "this", "that", "what", "which", "with", "from", "have", "been",
        "show", "mostra", "elenca", "tutti", "tutte", "all", "why", "perche", "causa",
        "motivo", "summary", "summarize", "riassumi", "sintesi", "log", "logs",
        "messaggi", "messaggio", "indice", "index", "riga", "line", "timestamp", "time",
        "tempo", "error", "errors", "errore", "errori", "fatal", "fatale", "warn",
        "warning", "avviso", "info", "debug", "verbose", "can", "not", "are", "was",
        "for", "will", "has", "had", "but", "its", "also"
    };
    QStringList result;
    result.reserve(tokens.size());
    for (const QString &t : tokens) {
        if (t.size() < 3) continue;
        if (stopwords.contains(t)) continue;
        if (t.at(0).isDigit()) continue;
        result.append(t);
    }
    result.removeDuplicates();
    return result;
}

QSet<int> findMatchingIndices(const QString &query,
                               const QVector<DltAnalyzerInterface::LogEntry> &entries,
                               const QHash<QString, QSet<int>> &invertedIndex)
{
    QStringList keywords = extractKeywordsSimple(query);
    if (keywords.isEmpty()) {
        QRegularExpression rx(QRegularExpression::escape(query),
                              QRegularExpression::CaseInsensitiveOption);
        if (!rx.isValid()) return {};
        QSet<int> matches;
        for (const auto &e : entries) {
            if (e.payload.contains(rx) || e.apid.contains(rx) ||
                e.ctid.contains(rx) || e.ecu.contains(rx) ||
                e.level.contains(rx) || e.domain.contains(rx))
                matches.insert(e.index);
        }
        return matches;
    }

    QSet<int> candidates;
    bool first = true;
    for (const QString &kw : keywords) {
        auto it = invertedIndex.find(kw);
        if (it == invertedIndex.end() || it->isEmpty())
            return {};
        if (first) {
            candidates = *it;
            first = false;
        } else {
            candidates.intersect(*it);
        }
        if (candidates.isEmpty()) return {};
    }

    QSet<int> results;
    QRegularExpression rx(QRegularExpression::escape(query),
                          QRegularExpression::CaseInsensitiveOption);
    for (const auto &e : entries) {
        if (!candidates.contains(e.index)) continue;
        if (e.payload.contains(rx) || e.apid.contains(rx) ||
            e.ctid.contains(rx) || e.ecu.contains(rx) ||
            e.level.contains(rx) || e.domain.contains(rx) ||
            e.time.contains(rx))
            results.insert(e.index);
    }
    return results;
}

QVector<ContextualExtractor::CtxGroup> ContextualExtractor::buildGroups(
    const QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    QHash<QString, CtxGroup> groupMap;
    for (const auto &e : entries) {
        QString key = QString("%1/%2/%3").arg(e.ecu, e.apid, e.ctid);
        auto it = groupMap.find(key);
        if (it == groupMap.end()) {
            CtxGroup g;
            g.ecu = e.ecu;
            g.apid = e.apid;
            g.ctid = e.ctid;
            g.groupKey = key;
            g.totalInGroup = 1;
            groupMap.insert(key, g);
        } else {
            it->totalInGroup++;
        }
    }
    QVector<CtxGroup> groups;
    groups.reserve(groupMap.size());
    for (auto it = groupMap.begin(); it != groupMap.end(); ++it)
        groups.append(it.value());
    // Sort by group size descending (most relevant first)
    std::sort(groups.begin(), groups.end(), [](const CtxGroup &a, const CtxGroup &b) {
        return a.totalInGroup > b.totalInGroup;
    });
    return groups;
}

QSet<int> ContextualExtractor::expandAnchors(
    const QVector<int> &anchorIndices,
    const QVector<DltAnalyzerInterface::LogEntry> &groupEntries,
    int windowBefore,
    int windowAfter) const
{
    QSet<int> result;
    if (groupEntries.isEmpty()) return result;

    // Build position lookup: index → position in groupEntries
    QHash<int, int> indexToPos;
    indexToPos.reserve(groupEntries.size());
    for (int i = 0; i < groupEntries.size(); ++i)
        indexToPos.insert(groupEntries[i].index, i);

    QSet<int> anchorSet(anchorIndices.begin(), anchorIndices.end());

    for (int anchorIdx : anchorIndices) {
        if (!indexToPos.contains(anchorIdx)) continue;
        int pos = indexToPos[anchorIdx];

        int start = qMax(0, pos - windowBefore);
        int end = qMin(groupEntries.size() - 1, pos + windowAfter);

        for (int i = start; i <= end; ++i) {
            result.insert(groupEntries[i].index);
        }
    }

    return result;
}

QVector<DltAnalyzerInterface::LogEntry> ContextualExtractor::sortByTimestamp(
    const QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    QVector<DltAnalyzerInterface::LogEntry> sorted = entries;
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
        return a.index < b.index;
    });
    return sorted;
}

QVector<DltAnalyzerInterface::LogEntry> ContextualExtractor::extractContext(
    const QString &query,
    const QVector<DltAnalyzerInterface::LogEntry> &allEntries,
    const QHash<QString, QSet<int>> &invertedIndex)
{
    ContextConfig defaultConfig;
    return extractContext(query, allEntries, invertedIndex, {}, defaultConfig);
}

QVector<DltAnalyzerInterface::LogEntry> ContextualExtractor::extractContext(
    const QString &query,
    const QVector<DltAnalyzerInterface::LogEntry> &allEntries,
    const QHash<QString, QSet<int>> &invertedIndex,
    const QList<int> &selectedIndices,
    const ContextConfig &config)
{
    m_lastStats = ExtractStats();
    m_lastGroups.clear();

    if (allEntries.isEmpty()) return {};

    // Step 1: Determine anchor indices
    QSet<int> anchorsSet;
    if (!selectedIndices.isEmpty()) {
        for (int idx : selectedIndices)
            anchorsSet.insert(idx);
    } else if (!query.isEmpty()) {
        anchorsSet = findMatchingIndices(query, allEntries, invertedIndex);
    }

    if (anchorsSet.isEmpty()) {
        m_lastStats.totalEntries = 0;
        return {};
    }

    m_lastStats.anchorCount = anchorsSet.size();

    // Step 2: Build groups from all entries
    m_lastGroups = buildGroups(allEntries);
    m_lastStats.groupsFound = m_lastGroups.size();

    // Step 3: Assign anchor indices to groups
    QHash<QString, QVector<int>> groupAnchorMap;
    for (const auto &e : allEntries) {
        if (!anchorsSet.contains(e.index)) continue;
        QString key = QString("%1/%2/%3").arg(e.ecu, e.apid, e.ctid);
        groupAnchorMap[key].append(e.index);
    }

    // Step 4: For each group with anchors, expand
    QSet<int> resultSet;
    for (const auto &group : m_lastGroups) {
        auto anchorIt = groupAnchorMap.find(group.groupKey);
        if (anchorIt == groupAnchorMap.end() || anchorIt->isEmpty())
            continue;

        // Collect all entries in this group
        QVector<DltAnalyzerInterface::LogEntry> groupEntries;
        for (const auto &e : allEntries) {
            QString key = QString("%1/%2/%3").arg(e.ecu, e.apid, e.ctid);
            if (key == group.groupKey)
                groupEntries.append(e);
        }

        // Expand anchors with window
        QSet<int> expanded = expandAnchors(anchorIt.value(), groupEntries,
                                            config.windowBefore, config.windowAfter);
        resultSet.unite(expanded);
    }

    if (resultSet.isEmpty()) {
        m_lastStats.totalEntries = 0;
        return {};
    }

    // Step 5: Collect LogEntry objects
    QVector<DltAnalyzerInterface::LogEntry> result;
    result.reserve(qMin(resultSet.size(), config.maxEntries));
    for (const auto &e : allEntries) {
        if (resultSet.contains(e.index)) {
            result.append(e);
            if (result.size() >= config.maxEntries) break;
        }
    }

    // Step 6: Sort by timestamp
    result = sortByTimestamp(result);

    m_lastStats.totalEntries = result.size();
    m_lastStats.expandedCount = resultSet.size();

    return result;
}
