#include "dltchat/temporal_correlator.h"

#include <algorithm>
#include <QHash>
#include <QMap>

namespace dltchat {

QString TemporalCorrelator::analyze(
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const CorrelationConfig &config)
{
    m_correlations.clear();
    if (entries.size() < 2) return QString();

    QVector<DltAnalyzerInterface::LogEntry> sorted = entries;
    std::sort(sorted.begin(), sorted.end(),
        [](const auto &a, const auto &b) { return a.timestamp < b.timestamp; });

    QMap<qint64, QVector<DltAnalyzerInterface::LogEntry>> windows;

    for (const auto &entry : sorted)
    {
        bool tsOk = false;
        qint64 ts = entry.timestamp.toLongLong(&tsOk);
        if (!tsOk)
        {
            ts = static_cast<qint64>(&entry - &sorted[0]);
        }
        qint64 windowKey = (ts / config.windowMs) * config.windowMs;
        windows[windowKey].append(entry);
    }

    int corrCount = 0;
    for (auto it = windows.begin(); it != windows.end() && corrCount < config.maxCorrelations; ++it)
    {
        if (it.value().size() >= config.minEntriesPerWindow)
        {
            Correlation corr;
            corr.windowStart = it.key();
            corr.windowEnd = it.key() + config.windowMs;
            corr.entries = it.value();

            QStringList ecus, apids;
            for (const auto &e : it.value())
            {
                if (!e.ecu.isEmpty() && !ecus.contains(e.ecu))
                    ecus.append(e.ecu);
                if (!e.apid.isEmpty() && !apids.contains(e.apid))
                    apids.append(e.apid);
            }

            QStringList parts;
            parts << QString("%1 entries").arg(it.value().size());
            if (!ecus.isEmpty())
                parts << QString("ECUs: %1").arg(ecus.join(", "));
            if (!apids.isEmpty())
                parts << QString("APIDs: %1").arg(apids.join(", "));
            corr.description = parts.join(" | ");

            m_correlations.append(corr);
            corrCount++;
        }
    }

    if (m_correlations.isEmpty())
        return QString();

    QString html = QString("<b>Temporal Correlations</b> (window=%1ms):<br>")
        .arg(config.windowMs);
    for (const auto &c : m_correlations)
    {
        html += QString("&bull; Window %1-%2: %3<br>")
            .arg(c.windowStart).arg(c.windowEnd).arg(c.description);
    }

    return html;
}

} // namespace dltchat
