#ifndef DLTCHAT_TEMPORAL_CORRELATOR_H
#define DLTCHAT_TEMPORAL_CORRELATOR_H

#include "analyzer_interface.h"
#include <QString>
#include <QVector>
#include <QSet>

namespace dltchat {

class TemporalCorrelator
{
public:
    struct CorrelationConfig {
        int windowMs;
        int minEntriesPerWindow;
        int maxCorrelations;

        CorrelationConfig()
            : windowMs(100)
            , minEntriesPerWindow(2)
            , maxCorrelations(5)
        {}
    };

    struct Correlation {
        qint64 windowStart;
        qint64 windowEnd;
        QVector<DltAnalyzerInterface::LogEntry> entries;
        QString description;
    };

    QString analyze(const QVector<DltAnalyzerInterface::LogEntry> &entries,
                    const CorrelationConfig &config = CorrelationConfig());
    bool hasCorrelations() const { return !m_correlations.isEmpty(); }
    QVector<Correlation> correlations() const { return m_correlations; }

private:
    QVector<Correlation> m_correlations;
};

} // namespace dltchat

#endif
