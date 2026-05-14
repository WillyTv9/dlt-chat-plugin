#include "test_fibexenricher.h"
#include "dltchat/fibex_enricher.h"
#include <QTest>
#include <QTemporaryFile>
#include <QDir>

using namespace dltchat;
using LogEntry = DltAnalyzerInterface::LogEntry;

static QString createTestXml()
{
    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QDir::tempPath() + "/fibex_test_XXXXXX.xml");
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open()) return QString();

    tmpFile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<FIBEX>\n"
        "    <APPLICATION>\n"
        "        <SHORT-NAME>PWRM_MAIN</SHORT-NAME>\n"
        "        <LONG-NAME>Power Management Main</LONG-NAME>\n"
        "        <FUNCTION-NAME>PowerStateManager</FUNCTION-NAME>\n"
        "    </APPLICATION>\n"
        "    <APPLICATION>\n"
        "        <SHORT-NAME>CANIF_RX</SHORT-NAME>\n"
        "        <LONG-NAME>CAN Interface Receive</LONG-NAME>\n"
        "        <FUNCTION-NAME>CanIf_Receive</FUNCTION-NAME>\n"
        "    </APPLICATION>\n"
        "    <FUNCTION>\n"
        "        <SHORT-NAME>SOMEIP_SD</SHORT-NAME>\n"
        "    </FUNCTION>\n"
        "</FIBEX>\n");
    tmpFile.close();
    return tmpFile.fileName();
}

void TestFibexEnricher::testNotLoadedInitially()
{
    FibexEnricher enricher;
    QVERIFY(!enricher.isLoaded());
    QCOMPARE(enricher.mappingCount(), 0);
}

void TestFibexEnricher::testEnrichNoData()
{
    FibexEnricher enricher;
    LogEntry e;
    e.index = 0; e.apid = "PWRM"; e.ctid = "MAIN";
    e.payload = "timeout detected";

    QString originalPayload = e.payload;
    QVector<LogEntry> entries = {e};
    enricher.enrichAll(entries);
    QCOMPARE(entries[0].payload, originalPayload);
}

void TestFibexEnricher::testClear()
{
    FibexEnricher enricher;
    QString testFile = createTestXml();
    if (testFile.isEmpty()) QSKIP("Cannot create temp file");

    bool loaded = enricher.loadFile(testFile);
    QVERIFY(loaded);
    QVERIFY(enricher.isLoaded());
    enricher.clear();
    QVERIFY(!enricher.isLoaded());
    QCOMPARE(enricher.mappingCount(), 0);
    QFile::remove(testFile);
}

void TestFibexEnricher::testEnrichEntry()
{
    FibexEnricher enricher;
    QString testFile = createTestXml();
    if (testFile.isEmpty()) QSKIP("Cannot create temp file");

    bool loaded = enricher.loadFile(testFile);
    QVERIFY(loaded);
    QVERIFY(enricher.isLoaded());
    QVERIFY(enricher.mappingCount() > 0);

    // Enrich a known entry
    LogEntry e;
    e.index = 0; e.apid = "PWRM"; e.ctid = "MAIN";
    e.payload = "timeout detected";

    QVector<LogEntry> entries = {e};
    enricher.enrichAll(entries);
    // Payload should be enriched with human-readable function name
    QVERIFY(entries[0].payload.contains("timeout detected"));
    QVERIFY(entries[0].payload.contains("PowerStateManager"));

    QFile::remove(testFile);
}

void TestFibexEnricher::testLoadInvalidFile()
{
    FibexEnricher enricher;
    QString error;
    bool loaded = enricher.loadFile("/nonexistent/file.fibex", &error);
    QVERIFY(!loaded);
    QVERIFY(!error.isEmpty());
}
