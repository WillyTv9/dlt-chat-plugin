#include "temporalcorrelator.h"
#include <QHash>
#include <QRegularExpression>
#include <algorithm>

int TemporalCorrelator::parseTimestampMs(const QString &timestamp) const
{
    // Format 1: "ssss.mmmm" (seconds.0.1ms-units, e.g., DLT Viewer timestamp format)
    static QRegularExpression secPattern("^(\\d+)\\.(\\d{1,4})$");
    auto match = secPattern.match(timestamp);
    if (match.hasMatch()) {
        int seconds = match.captured(1).toInt();
        int frac = match.captured(2).toInt();
        // frac is in 1/10000ths of a second (0.1ms units), convert to ms
        // e.g., 200 units = 20ms
        return seconds * 1000 + frac / 10;
    }

    // Format 2: "HH:mm:ss.zzz"
    static QRegularExpression timePattern("^(\\d+):(\\d{2}):(\\d{2})\\.(\\d+)$");
    match = timePattern.match(timestamp);
    if (match.hasMatch()) {
        int h = match.captured(1).toInt();
        int m = match.captured(2).toInt();
        int s = match.captured(3).toInt();
        QString fracStr = match.captured(4);
        // Convert fractional part to milliseconds
        int ms = 0;
        if (fracStr.length() >= 3)
            ms = fracStr.left(3).toInt();
        else if (fracStr.length() == 2)
            ms = fracStr.toInt() * 10;
        else if (fracStr.length() == 1)
            ms = fracStr.toInt() * 100;
        return (h * 3600 + m * 60 + s) * 1000 + ms;
    }

    // Format 3: plain number (milliseconds already)
    bool ok = false;
    int val = timestamp.toInt(&ok);
    if (ok)
        return val;

    return 0;
}

void TemporalCorrelator::addToWindow(
    QHash<int, QVector<CorrelationEvent>> &windows,
    const CorrelationEvent &event,
    int windowMs) const
{
    int windowKey = (event.timestampMs / windowMs) * windowMs;
    windows[windowKey].append(event);
}

QString TemporalCorrelator::analyze(
    const QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    CorrelationConfig defaultConfig;
    return analyze(entries, defaultConfig);
}

QString TemporalCorrelator::analyze(
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const CorrelationConfig &config) const
{
    m_lastGroups.clear();

    if (entries.size() < 2)
        return QString();

    // Step 1: Parse timestamps and build windows
    QHash<int, QVector<CorrelationEvent>> windows;

    for (const auto &e : entries) {
        CorrelationEvent ce;
        ce.index = e.index;
        ce.ecu = e.ecu;
        ce.apid = e.apid;
        ce.ctid = e.ctid;
        ce.level = e.level;
        ce.payload = e.payload.left(80);
        ce.timestampMs = parseTimestampMs(e.timestamp);

        addToWindow(windows, ce, config.windowMs);
    }

    // Step 2: Find windows with multiple ECUs
    QStringList correlationLines;

    for (auto it = windows.constBegin(); it != windows.constEnd(); ++it) {
        if (correlationLines.size() >= config.maxCorrelations)
            break;

        const auto &events = it.value();
        if (events.size() < config.minEntriesPerWindow)
            continue;

        // Check if multiple ECUs in this window
        QSet<QString> ecusInWindow;
        for (const auto &ev : events)
            ecusInWindow.insert(ev.ecu);

        if (ecusInWindow.size() < 2)
            continue;

        // Build correlation group
        CorrelationGroup group;
        group.windowStartMs = it.key();
        group.windowEndMs = it.key() + config.windowMs;
        group.ecus = ecusInWindow.values();
        group.events = events;

        // Sort events within window by timestamp
        std::sort(group.events.begin(), group.events.end(),
            [](const CorrelationEvent &a, const CorrelationEvent &b) {
                return a.timestampMs < b.timestampMs;
            });

        m_lastGroups.append(group);

        // Build human-readable line
        QStringList ecuList = ecusInWindow.values();
        std::sort(ecuList.begin(), ecuList.end());

        QStringList eventLines;
        for (const auto &ev : group.events) {
            // Highlight errors/fatals
            QString levelTag;
            if (ev.level == "error" || ev.level == "fatal")
                levelTag = " ⚠";
            eventLines.append(QString("    [index:%1] %2 | %3/%4 | %5%6")
                .arg(ev.index).arg(ev.ecu).arg(ev.apid).arg(ev.ctid)
                .arg(ev.payload.left(60)).arg(levelTag));
        }

        correlationLines.append(QString(
            "Window %1-%2ms | ECUs: %3 | %4 events:\n%5")
            .arg(group.windowStartMs).arg(group.windowEndMs)
            .arg(ecuList.join(", "))
            .arg(events.size())
            .arg(eventLines.join("\n")));
    }

    if (correlationLines.isEmpty())
        return QString();

    return QString("Temporal Correlation (%1ms window):\n%2")
        .arg(config.windowMs)
        .arg(correlationLines.join("\n\n"));
}
