#include "test_temporalcorrelator.h"
#include "dltchat/temporal_correlator.h"
#include <QTest>

using namespace dltchat;
using LogEntry = DltAnalyzerInterface::LogEntry;

static LogEntry makeEntry(int index, const QString &timestamp,
                          const QString &ecu, const QString &apid,
                          const QString &ctid, const QString &level,
                          const QString &payload)
{
    LogEntry e;
    e.index = index;
    e.timestamp = timestamp;
    e.time = timestamp;
    e.ecu = ecu;
    e.apid = apid;
    e.ctid = ctid;
    e.level = level;
    e.payload = payload;
    e.domain = "generic";
    return e;
}

void TestTemporalCorrelator::testNoEntriesReturnsEmpty()
{
    TemporalCorrelator correlator;
    QVector<LogEntry> entries;
    QString result = correlator.analyze(entries);
    QVERIFY(result.isEmpty());
    QVERIFY(!correlator.hasCorrelations());
}

void TestTemporalCorrelator::testSingleEntryReturnsEmpty()
{
    TemporalCorrelator correlator;
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "1000.0000", "ECU1", "APP", "CTX", "info", "test"));
    QString result = correlator.analyze(entries);
    QVERIFY(result.isEmpty());
}

void TestTemporalCorrelator::testTwoEcusInWindow()
{
    TemporalCorrelator correlator;
    QVector<LogEntry> entries;
    // Both within the same 50ms window
    entries.append(makeEntry(0, "1000.0000", "ECU1", "APP", "CTX", "error", "timeout"));
    entries.append(makeEntry(1, "1000.0200", "ECU2", "APP", "CTX", "error", "bus off"));

    TemporalCorrelator::CorrelationConfig config;
    config.windowMs = 50;
    config.minEntriesPerWindow = 2;

    QString result = correlator.analyze(entries, config);
    QVERIFY(!result.isEmpty());
    QVERIFY(correlator.hasCorrelations());
    QVERIFY(result.contains("ECU1"));
    QVERIFY(result.contains("ECU2"));
    QVERIFY(result.contains("Temporal Correlation"));

    // Check groups
    auto groups = correlator.correlations();
    QCOMPARE(groups.size(), 1);
    QCOMPARE(groups[0].entries.size(), 2);
}

void TestTemporalCorrelator::testNoCorrelationWhenEcusDistant()
{
    TemporalCorrelator correlator;
    QVector<LogEntry> entries;
    // 200ms apart, window is 50ms
    entries.append(makeEntry(0, "1000.0000", "ECU1", "APP", "CTX", "info", "a"));
    entries.append(makeEntry(1, "1200.0000", "ECU2", "APP", "CTX", "info", "b"));

    TemporalCorrelator::CorrelationConfig config;
    config.windowMs = 50;
    config.minEntriesPerWindow = 2;

    QString result = correlator.analyze(entries, config);
    QVERIFY(result.isEmpty());
    QVERIFY(!correlator.hasCorrelations());
}

void TestTemporalCorrelator::testParseTimestampSeconds()
{
    TemporalCorrelator correlator;

    // Access the parseTimestampMs through analyze with known timestamps
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "1000.0000", "ECU1", "A", "B", "info", "a"));
    entries.append(makeEntry(1, "1000.0500", "ECU2", "A", "B", "info", "b"));

    TemporalCorrelator::CorrelationConfig config;
    config.windowMs = 100; // 100ms window should capture both
    config.minEntriesPerWindow = 2;

    QString result = correlator.analyze(entries, config);
    QVERIFY(!result.isEmpty()); // Both in same 100ms window

    config.windowMs = 20; // 20ms window should separate them
    result = correlator.analyze(entries, config);
    QVERIFY(result.isEmpty()); // Not in same 20ms window
}

void TestTemporalCorrelator::testParseTimestampTimeFormat()
{
    TemporalCorrelator correlator;
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "00:00:01.000000", "ECU1", "A", "B", "info", "a"));
    entries.append(makeEntry(1, "00:00:01.020000", "ECU2", "A", "B", "error", "b"));

    TemporalCorrelator::CorrelationConfig config;
    config.windowMs = 50;
    config.minEntriesPerWindow = 2;

    QString result = correlator.analyze(entries, config);
    QVERIFY(!result.isEmpty());
    QVERIFY(result.contains("ECU1"));
    QVERIFY(result.contains("ECU2"));
}

void TestTemporalCorrelator::testParseTimestampPlainNumber()
{
    TemporalCorrelator correlator;
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "1000", "ECU1", "A", "B", "info", "a"));
    entries.append(makeEntry(1, "1020", "ECU2", "A", "B", "error", "b"));

    TemporalCorrelator::CorrelationConfig config;
    config.windowMs = 50;
    config.minEntriesPerWindow = 2;

    QString result = correlator.analyze(entries, config);
    QVERIFY(!result.isEmpty());
    QVERIFY(result.contains("ECU1"));
    QVERIFY(result.contains("ECU2"));
}
