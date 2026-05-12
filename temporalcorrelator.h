#ifndef TEMPORALCORRELATOR_H
#define TEMPORALCORRELATOR_H

#include "dltanalyzerinterface.h"
#include <QString>
#include <QVector>
#include <QSet>

class TemporalCorrelator
{
public:
    struct CorrelationConfig
    {
        int windowMs = 50;
        int minEntriesPerWindow = 2;
        int maxCorrelations = 5;
    };

    struct CorrelationEvent
    {
        int index = -1;
        QString ecu;
        QString apid;
        QString ctid;
        QString level;
        QString payload;
        int timestampMs = 0;
    };

    struct CorrelationGroup
    {
        int windowStartMs = 0;
        int windowEndMs = 0;
        QStringList ecus;
        QVector<CorrelationEvent> events;
    };

    // Analyze entries for temporal correlations across ECUs
    QString analyze(const QVector<DltAnalyzerInterface::LogEntry> &entries,
                    const CorrelationConfig &config) const;
    QString analyze(const QVector<DltAnalyzerInterface::LogEntry> &entries) const;

    // Get detailed correlation groups (for inspection)
    QVector<CorrelationGroup> correlationGroups() const { return m_lastGroups; }

    // Check if any correlations were found
    bool hasCorrelations() const { return !m_lastGroups.isEmpty(); }

private:
    int parseTimestampMs(const QString &timestamp) const;
    void addToWindow(QHash<int, QVector<CorrelationEvent>> &windows,
                     const CorrelationEvent &event, int windowMs) const;

    mutable QVector<CorrelationGroup> m_lastGroups;
};

#endif
