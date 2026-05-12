#include "test_contextualextractor.h"
#include "contextualextractor.h"
#include <QTest>
#include <QSet>

using LogEntry = DltAnalyzerInterface::LogEntry;

// Helper to create a LogEntry with all essential fields
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

void TestContextualExtractor::testExtractContextWithSelectedIndices()
{
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "100", "ECU1", "APP1", "CTX1", "error", "timeout"));
    entries.append(makeEntry(1, "200", "ECU1", "APP1", "CTX1", "info", "retry"));
    entries.append(makeEntry(2, "300", "ECU1", "APP1", "CTX1", "info", "success"));
    entries.append(makeEntry(3, "400", "ECU1", "APP2", "CTX2", "error", "memory fault"));
    entries.append(makeEntry(4, "500", "ECU1", "APP2", "CTX2", "warn", "high usage"));

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;
    ContextualExtractor::ContextConfig config;
    config.windowBefore = 1;
    config.windowAfter = 1;
    config.maxEntries = 200;

    // Select index 0 (the timeout error), should get 0 + 1 before/after within same group
    QList<int> selected = {0};
    auto result = extractor.extractContext("", entries, invertedIndex, selected, config);

    QVERIFY(!result.isEmpty());
    QVERIFY(result.size() <= 3); // 0 + 1 before + 1 after = max 3 in APP1/CTX1
    // Index 0 is anchor, index 1 is after, no index -1 so only 2 entries
    // Actually: windowBefore=1 means pos-1=0 → 0, pos+1=1 → includes 1
    QCOMPARE(result.size(), 2);
    QVERIFY(result[0].index == 0 || result[0].index == 1);
}

void TestContextualExtractor::testExtractContextNoQueryReturnsEmpty()
{
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "100", "ECU1", "APP1", "CTX1", "error", "test"));

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;

    // No query and no selected indices → should return empty
    auto result = extractor.extractContext("", entries, invertedIndex);
    QVERIFY(result.isEmpty());
}

void TestContextualExtractor::testWindowExpansion()
{
    QVector<LogEntry> entries;
    // 10 entries in the same group
    for (int i = 0; i < 10; ++i) {
        entries.append(makeEntry(i, QString::number(i * 100), "ECU1", "APP", "CTX",
                                 i == 5 ? "error" : "info",
                                 QString("payload %1").arg(i)));
    }

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;
    ContextualExtractor::ContextConfig config;
    config.windowBefore = 2;
    config.windowAfter = 2;
    config.maxEntries = 200;

    // Anchor is entry 5 (error)
    QList<int> selected = {5};
    auto result = extractor.extractContext("", entries, invertedIndex, selected, config);

    // Should include indices 3,4,5,6,7 (2 before, 2 after, 5 is anchor)
    QSet<int> expected = {3, 4, 5, 6, 7};
    QCOMPARE(result.size(), 5);
    for (const auto &e : result) {
        QVERIFY(expected.contains(e.index));
    }
}

void TestContextualExtractor::testGroupsBuiltCorrectly()
{
    QVector<LogEntry> entries;
    entries.append(makeEntry(0, "100", "ECU1", "APP1", "CTX1", "info", "a"));
    entries.append(makeEntry(1, "200", "ECU1", "APP1", "CTX2", "info", "b"));
    entries.append(makeEntry(2, "300", "ECU2", "APP1", "CTX1", "info", "c"));
    entries.append(makeEntry(3, "400", "ECU1", "APP1", "CTX1", "info", "d"));

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;
    ContextualExtractor::ContextConfig config;
    config.windowBefore = 0;
    config.windowAfter = 0;

    QList<int> selected = {0};
    auto result = extractor.extractContext("", entries, invertedIndex, selected, config);

    QVERIFY(!result.isEmpty());
    auto groups = extractor.groups();

    // Should have 3 groups
    QCOMPARE(groups.size(), 3);

    // ECU1/APP1/CTX1 should have totalInGroup = 2
    bool foundECU1APP1CTX1 = false;
    bool foundECU1APP1CTX2 = false;
    bool foundECU2APP1CTX1 = false;
    for (const auto &g : groups) {
        if (g.groupKey == "ECU1/APP1/CTX1") {
            foundECU1APP1CTX1 = true;
            QCOMPARE(g.totalInGroup, 2);
        }
        if (g.groupKey == "ECU1/APP1/CTX2") {
            foundECU1APP1CTX2 = true;
            QCOMPARE(g.totalInGroup, 1);
        }
        if (g.groupKey == "ECU2/APP1/CTX1") {
            foundECU2APP1CTX1 = true;
            QCOMPARE(g.totalInGroup, 1);
        }
    }
    QVERIFY(foundECU1APP1CTX1);
    QVERIFY(foundECU1APP1CTX2);
    QVERIFY(foundECU2APP1CTX1);
}

void TestContextualExtractor::testSortByTimestamp()
{
    QVector<LogEntry> entries;
    entries.append(makeEntry(5, "500", "ECU1", "APP", "CTX", "info", "last"));
    entries.append(makeEntry(1, "100", "ECU1", "APP", "CTX", "info", "first"));
    entries.append(makeEntry(3, "300", "ECU1", "APP", "CTX", "info", "middle"));

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;
    ContextualExtractor::ContextConfig config;
    config.windowBefore = 5;
    config.windowAfter = 5;

    QList<int> selected = {1, 3, 5};
    auto result = extractor.extractContext("", entries, invertedIndex, selected, config);

    QCOMPARE(result.size(), 3);
    QCOMPARE(result[0].index, 1);
    QCOMPARE(result[1].index, 3);
    QCOMPARE(result[2].index, 5);
}

void TestContextualExtractor::testMaxEntriesLimit()
{
    QVector<LogEntry> entries;
    for (int i = 0; i < 50; ++i) {
        entries.append(makeEntry(i, QString::number(i * 100), "ECU1", "APP", "CTX",
                                 "info", QString("payload %1").arg(i)));
    }

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;
    ContextualExtractor::ContextConfig config;
    config.windowBefore = 10;
    config.windowAfter = 10;
    config.maxEntries = 10;

    QList<int> selected = {25};
    auto result = extractor.extractContext("", entries, invertedIndex, selected, config);

    QVERIFY(result.size() <= 10);
}

void TestContextualExtractor::testMultipleCtxGroups()
{
    QVector<LogEntry> entries;
    // Group A: ECU1/APP1/CTX1  (indices 0-4)
    for (int i = 0; i < 5; ++i)
        entries.append(makeEntry(i, QString::number(i * 100), "ECU1", "APP1", "CTX1",
                                 "info", QString("a%1").arg(i)));
    // Group B: ECU1/APP1/CTX2  (indices 5-9)
    for (int i = 5; i < 10; ++i)
        entries.append(makeEntry(i, QString::number(i * 100), "ECU1", "APP1", "CTX2",
                                 "info", QString("b%1").arg(i)));
    // Group C: ECU2/APP2/CTX3  (indices 10-14)
    for (int i = 10; i < 15; ++i)
        entries.append(makeEntry(i, QString::number(i * 100), "ECU2", "APP2", "CTX3",
                                 "info", QString("c%1").arg(i)));

    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;
    ContextualExtractor::ContextConfig config;
    config.windowBefore = 1;
    config.windowAfter = 1;
    config.maxEntries = 200;

    // Select anchors from Group A (index 2) and Group C (index 12)
    QList<int> selected = {2, 12};
    auto result = extractor.extractContext("", entries, invertedIndex, selected, config);

    // Should include indices from both groups
    QSet<int> resultIndices;
    for (const auto &e : result)
        resultIndices.insert(e.index);

    // Group A: anchor 2 + 1 before(1) + 1 after(3) = {1,2,3}
    QVERIFY(resultIndices.contains(1));
    QVERIFY(resultIndices.contains(2));
    QVERIFY(resultIndices.contains(3));

    // Group C: anchor 12 + 1 before(11) + 1 after(13) = {11,12,13}
    QVERIFY(resultIndices.contains(11));
    QVERIFY(resultIndices.contains(12));
    QVERIFY(resultIndices.contains(13));

    // Should NOT contain indices from Group B (no anchors there)
    QVERIFY(!resultIndices.contains(7));
}

void TestContextualExtractor::testEmptyEntriesReturnsEmpty()
{
    QVector<LogEntry> entries;
    QHash<QString, QSet<int>> invertedIndex;
    ContextualExtractor extractor;

    auto result = extractor.extractContext("error", entries, invertedIndex);
    QVERIFY(result.isEmpty());
}
