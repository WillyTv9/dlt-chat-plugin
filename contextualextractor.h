#ifndef CONTEXTUALEXTRACTOR_H
#define CONTEXTUALEXTRACTOR_H

#include "dltanalyzerinterface.h"
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

class ContextualExtractor
{
public:
    struct ContextConfig
    {
        int windowBefore = 5;
        int windowAfter = 5;
        int maxEntries = 200;
        bool enableTemporalCorrelation = false;
    };

    struct CtxGroup
    {
        QString ecu;
        QString apid;
        QString ctid;
        QString groupKey;
        QVector<int> anchorIndices;
        int totalInGroup = 0;
    };

    struct ExtractStats
    {
        int totalEntries = 0;
        int groupsFound = 0;
        int anchorCount = 0;
        int expandedCount = 0;
    };

    QVector<DltAnalyzerInterface::LogEntry> extractContext(
        const QString &query,
        const QVector<DltAnalyzerInterface::LogEntry> &allEntries,
        const QHash<QString, QSet<int>> &invertedIndex,
        const QList<int> &selectedIndices,
        const ContextConfig &config);

    QVector<DltAnalyzerInterface::LogEntry> extractContext(
        const QString &query,
        const QVector<DltAnalyzerInterface::LogEntry> &allEntries,
        const QHash<QString, QSet<int>> &invertedIndex);

    QVector<CtxGroup> groups() const { return m_lastGroups; }
    ExtractStats lastStats() const { return m_lastStats; }

private:
    struct InternalEntry {
        int index = -1;
        QString ecu;
        QString apid;
        QString ctid;
        QString groupKey;
    };

    QVector<CtxGroup> buildGroups(
        const QVector<DltAnalyzerInterface::LogEntry> &entries) const;

    QSet<int> expandAnchors(
        const QVector<int> &anchorIndices,
        const QVector<DltAnalyzerInterface::LogEntry> &groupEntries,
        int windowBefore,
        int windowAfter) const;

    QVector<DltAnalyzerInterface::LogEntry> sortByTimestamp(
        const QVector<DltAnalyzerInterface::LogEntry> &entries) const;

    mutable ExtractStats m_lastStats;
    mutable QVector<CtxGroup> m_lastGroups;
};

#endif
