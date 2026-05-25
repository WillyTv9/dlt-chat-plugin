#include "test_hierarchical_summary_store.h"

#include "dltchat/hierarchical_summary_store.h"

#include <QTest>

using namespace dltchat;

void TestHierarchicalSummaryStore::init()
{
}

void TestHierarchicalSummaryStore::cleanup()
{
}

void TestHierarchicalSummaryStore::testSetAndGetStatistics()
{
    HierarchicalSummaryStore store;
    QVERIFY(!store.isReady());

    LogStatistics stats;
    stats.totalEntries = 42;
    stats.byEcu["ECU1"] = 30;
    stats.byEcu["ECU2"] = 12;
    stats.byApid["APP1"] = 25;
    stats.byLevel["error"] = 5;
    stats.byLevel["info"] = 37;
    stats.byCategory["Comunicazione"] = 3;
    stats.byDomain["carplay"] = 10;
    stats.firstTimestampMs = 1000;
    stats.lastTimestampMs = 5000;

    store.setStatistics(stats);
    QVERIFY(store.isReady());

    LogStatistics got = store.statistics();
    QCOMPARE(got.totalEntries, 42);
    QCOMPARE(got.byEcu.value("ECU1"), 30);
    QCOMPARE(got.byEcu.value("ECU2"), 12);
    QCOMPARE(got.byApid.value("APP1"), 25);
    QCOMPARE(got.byLevel.value("error"), 5);
    QCOMPARE(got.byCategory.value("Comunicazione"), 3);
    QCOMPARE(got.byDomain.value("carplay"), 10);
    QCOMPARE(got.firstTimestampMs, qint64(1000));
    QCOMPARE(got.lastTimestampMs, qint64(5000));

    store.clear();
    QVERIFY(!store.isReady());
    QCOMPARE(store.statistics().totalEntries, 0);
}

void TestHierarchicalSummaryStore::testCompactDigestEmpty()
{
    HierarchicalSummaryStore store;
    QString digest = store.compactDigest(1024);
    QVERIFY(digest.isEmpty());
}

void TestHierarchicalSummaryStore::testCompactDigestPopulated()
{
    HierarchicalSummaryStore store;

    LogStatistics stats;
    stats.totalEntries = 100;
    stats.byEcu["ECU1"] = 70;
    stats.byEcu["ECU2"] = 30;
    stats.byLevel["error"] = 12;
    stats.byLevel["info"] = 88;
    stats.firstTimestampMs = 0;
    stats.lastTimestampMs = 10000;
    store.setStatistics(stats);

    QVector<BlockSummary> blocks;
    BlockSummary b;
    b.firstIdx = 0;
    b.lastIdx = 49;
    b.startMs = 0;
    b.endMs = 5000;
    b.summary = "Block of normal startup activity";
    b.topApids = QStringList{"APP1", "APP2"};
    blocks.append(b);
    BlockSummary b2;
    b2.firstIdx = 50;
    b2.lastIdx = 99;
    b2.startMs = 5000;
    b2.endMs = 10000;
    b2.summary = "Block with errors";
    blocks.append(b2);
    store.setBlocks(blocks);

    QString digest = store.compactDigest(4096);
    QVERIFY(!digest.isEmpty());
    QVERIFY(digest.contains("[STATS]"));
    QVERIFY(digest.contains("entries=100"));
    QVERIFY(digest.contains("[ECUS]"));
    QVERIFY(digest.contains("ECU1"));
    QVERIFY(digest.contains("[LEVELS]"));
    QVERIFY(digest.contains("[BLOCKS]"));
}

void TestHierarchicalSummaryStore::testCompactDigestTruncation()
{
    HierarchicalSummaryStore store;

    LogStatistics stats;
    stats.totalEntries = 100;
    for (int i = 0; i < 20; ++i)
        stats.byApid[QString("APID_%1").arg(i)] = i + 1;
    for (int i = 0; i < 20; ++i)
        stats.byEcu[QString("ECU_%1").arg(i)] = i + 1;
    stats.firstTimestampMs = 0;
    stats.lastTimestampMs = 999999;
    store.setStatistics(stats);

    const int budget = 200;
    QString digest = store.compactDigest(budget);
    QVERIFY(!digest.isEmpty());
    QVERIFY2(digest.size() <= budget,
             qPrintable(QString("digest=%1 budget=%2").arg(digest.size()).arg(budget)));

    QString empty = store.compactDigest(0);
    QVERIFY(empty.isEmpty());
}

void TestHierarchicalSummaryStore::testByTimeWindow()
{
    HierarchicalSummaryStore store;

    LogStatistics stats;
    stats.totalEntries = 30;
    store.setStatistics(stats);

    QVector<BlockSummary> blocks;
    BlockSummary b1;
    b1.firstIdx = 0;
    b1.lastIdx = 9;
    b1.startMs = 500;
    b1.endMs = 1500;
    b1.summary = "first";
    blocks.append(b1);

    BlockSummary b2;
    b2.firstIdx = 10;
    b2.lastIdx = 19;
    b2.startMs = 2500;
    b2.endMs = 3500;
    b2.summary = "second";
    blocks.append(b2);

    BlockSummary b3;
    b3.firstIdx = 20;
    b3.lastIdx = 29;
    b3.startMs = 7500;
    b3.endMs = 8500;
    b3.summary = "third";
    blocks.append(b3);

    store.setBlocks(blocks);

    auto windows = store.byTimeWindow(5000);
    // b1 (500) and b2 (2500) bucket together at key 0; b3 (7500) at key 5000.
    QCOMPARE(windows.size(), 2);
    QCOMPARE(windows[0].startMs, qint64(0));
    QCOMPARE(windows[0].endMs, qint64(5000));
    QCOMPARE(windows[0].entryCount, 20);
    QVERIFY(windows[0].synthesis.contains("first"));
    QVERIFY(windows[0].synthesis.contains("second"));

    QCOMPARE(windows[1].startMs, qint64(5000));
    QCOMPARE(windows[1].endMs, qint64(10000));
    QCOMPARE(windows[1].entryCount, 10);
    QVERIFY(windows[1].synthesis.contains("third"));

    QVERIFY(store.byTimeWindow(0).isEmpty());
    QVERIFY(store.byTimeWindow(-100).isEmpty());
}
