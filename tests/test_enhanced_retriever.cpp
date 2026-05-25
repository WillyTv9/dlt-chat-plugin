#include "test_enhanced_retriever.h"

#include "dltchat/enhanced_retriever.h"

#include <QTest>

using namespace dltchat;

DltAnalyzerInterface::LogEntry TestEnhancedRetriever::makeEntry(int idx,
                                                                const QString &payload,
                                                                const QString &cat,
                                                                const QString &apid) const
{
    DltAnalyzerInterface::LogEntry e;
    e.index = idx;
    e.payload = payload;
    e.category = cat;
    e.apid = apid.isEmpty() ? QString("APP") : apid;
    e.ctid = "CTX";
    e.ecu = "ECU1";
    e.level = "info";
    e.timestamp = QString::number(1000 + idx);
    e.time = QString("10:00:00.%1").arg(idx, 3, 10, QLatin1Char('0'));
    return e;
}

void TestEnhancedRetriever::init()
{
}

void TestEnhancedRetriever::cleanup()
{
}

void TestEnhancedRetriever::testEmptyEntries()
{
    EnhancedRetriever r;
    QHash<QString, QSet<int>> idx;
    auto result = r.extract("bluetooth", {}, idx);
    QVERIFY(result.entries.isEmpty());
    QVERIFY(result.seedIndices.isEmpty());
    QVERIFY(!result.diagnostic.isEmpty());
}

void TestEnhancedRetriever::testTokenMatchesRetrieved()
{
    EnhancedRetriever r;
    QVector<DltAnalyzerInterface::LogEntry> entries;
    entries.append(makeEntry(0, "Service started successfully", "system", "APP_A"));
    entries.append(makeEntry(1, "Bluetooth pairing request received", "comms", "APP_B"));
    entries.append(makeEntry(2, "Heartbeat tick", "heartbeat", "APP_C"));
    entries.append(makeEntry(3, "GPS signal acquired", "gps", "APP_D"));
    entries.append(makeEntry(4, "User input event", "ui", "APP_E"));

    QHash<QString, QSet<int>> invertedIndex;
    invertedIndex["bluetooth"].insert(1);
    invertedIndex["pairing"].insert(1);
    invertedIndex["service"].insert(0);
    invertedIndex["heartbeat"].insert(2);
    invertedIndex["gps"].insert(3);

    EnhancedRetriever::Config cfg;
    cfg.windowBefore = 1;
    cfg.windowAfter = 1;

    auto result = r.extract("bluetooth", entries, invertedIndex, {}, cfg);

    QCOMPARE(result.seedIndices.size(), 1);
    QCOMPARE(result.seedIndices.first(), 1);

    // Window expansion ±1 around position 1 should pull in 0,1,2.
    QVERIFY(result.entries.size() >= 3);
    QSet<int> got;
    for (const auto &e : result.entries)
        got.insert(e.index);
    QVERIFY(got.contains(0));
    QVERIFY(got.contains(1));
    QVERIFY(got.contains(2));
}

void TestEnhancedRetriever::testDiversityPenalty()
{
    EnhancedRetriever r;
    QVector<DltAnalyzerInterface::LogEntry> entries;
    // 5 entries that all share the same (category|apid|ctid) diversity key
    // and all match the query "heartbeat".
    for (int i = 0; i < 5; ++i)
        entries.append(makeEntry(i, QString("heartbeat tick %1").arg(i), "system", "APP_HB"));

    QHash<QString, QSet<int>> invertedIndex;
    for (int i = 0; i < 5; ++i)
        invertedIndex["heartbeat"].insert(i);

    EnhancedRetriever::Config cfg;
    cfg.windowBefore = 0;
    cfg.windowAfter = 0;
    cfg.wDiversityPenalty = 0.8;

    auto result = r.extract("heartbeat", entries, invertedIndex, {}, cfg);
    // With diversity penalty on, only the first instance of the duplicate
    // (category|apid|ctid) key survives MMR.
    QCOMPARE(result.seedIndices.size(), 1);
}

void TestEnhancedRetriever::testIdfWeighting()
{
    EnhancedRetriever r;
    QVector<DltAnalyzerInterface::LogEntry> entries;
    // "info" is a very common token; "bluetooth" appears only once.
    for (int i = 0; i < 10; ++i)
        entries.append(makeEntry(i, "info heartbeat tick", "system",
                                  QString("APP_%1").arg(i)));
    entries.append(makeEntry(10, "info bluetooth pairing", "comms", "APP_BT"));

    QHash<QString, QSet<int>> invertedIndex;
    for (int i = 0; i < 11; ++i)
        invertedIndex["info"].insert(i);
    invertedIndex["bluetooth"].insert(10);

    EnhancedRetriever::Config cfg;
    cfg.windowBefore = 0;
    cfg.windowAfter = 0;
    cfg.wDiversityPenalty = 0.0; // disable MMR collapse so we see ranking
    cfg.maxEntries = 11;

    auto result = r.extract("info bluetooth", entries, invertedIndex, {}, cfg);

    // The rare-token-bearing entry (index 10) must rank first thanks to higher IDF.
    QVERIFY(!result.seedIndices.isEmpty());
    QCOMPARE(result.seedIndices.first(), 10);
}
