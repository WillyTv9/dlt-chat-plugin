#include "test_dltexport.h"
#include "dltexport.h"
#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

QVector<DltAnalyzerInterface::LogEntry> TestDltExport::makeEntries() const
{
    QVector<DltAnalyzerInterface::LogEntry> entries;
    entries.reserve(3);

    DltAnalyzerInterface::LogEntry e1;
    e1.index = 100; e1.time = "10:00:00.000000"; e1.timestamp = "12345678";
    e1.level = "error"; e1.ecu = "ECU1"; e1.apid = "APP1"; e1.ctid = "CTX1";
    e1.domain = "generic"; e1.payload = "Timeout error";
    entries.append(e1);

    DltAnalyzerInterface::LogEntry e2;
    e2.index = 200; e2.time = "10:00:01.000000"; e2.timestamp = "12345679";
    e2.level = "warn"; e2.ecu = "ECU2"; e2.apid = "APP2"; e2.ctid = "CTX2";
    e2.domain = "carplay"; e2.payload = "CarPlay disconnection";
    entries.append(e2);

    DltAnalyzerInterface::LogEntry e3;
    e3.index = 300; e3.time = "10:00:02.000000"; e3.timestamp = "12345680";
    e3.level = "info"; e3.ecu = "ECU1"; e3.apid = "APP3"; e3.ctid = "CTX3";
    e3.domain = "androidauto"; e3.payload = "Android Auto session started";
    entries.append(e3);

    return entries;
}

void TestDltExport::testCsvRowGeneration()
{
    QStringList fields = {"a", "b", "c"};
    QString row = DltExport::generateCsvRow(fields);
    QCOMPARE(row, "a,b,c");
}

void TestDltExport::testCsvRowEscaping()
{
    QStringList fields = {"hello", "contains,comma", "has\"quote", "multi\nline"};
    QString row = DltExport::generateCsvRow(fields);
    QVERIFY(row.contains("\"contains,comma\""));
    QVERIFY(row.contains("\"has\"\"quote\""));
    QVERIFY(row.contains("\"multi\nline\""));
}

void TestDltExport::testSanitizeFields()
{
    QStringList fields = {"  hello  ", "  world  "};
    QStringList sanitized = DltExport::sanitizeFields(fields);
    QCOMPARE(sanitized[0], "hello");
    QCOMPARE(sanitized[1], "world");
}

void TestDltExport::testExportToCsv()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.path() + "/test_export.csv";

    auto entries = makeEntries();
    QList<int> indices = {100, 200};
    QStringList snippets = {"Timeout error", "CarPlay disconnection"};
    QString query = "error";

    bool ok = DltExport::exportToCsv(path, indices, snippets, query, entries);
    QVERIFY(ok);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QVERIFY(content.contains("Timeout error"));
    QVERIFY(content.contains("CarPlay disconnection"));
    QVERIFY(content.contains("error"));  // source query
    QVERIFY(content.contains("Index"));
    QVERIFY(content.contains("Timestamp"));
    QVERIFY(content.contains("Domain"));
}

void TestDltExport::testExportAllEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.path() + "/test_export_all.csv";

    auto entries = makeEntries();
    bool ok = DltExport::exportAllEntries(path, entries);
    QVERIFY(ok);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QVERIFY(content.contains("Timeout error"));
    QVERIFY(content.contains("CarPlay disconnection"));
    QVERIFY(content.contains("Android Auto session started"));
    QVERIFY(content.contains("carplay"));
    QVERIFY(content.contains("androidauto"));
}

void TestDltExport::testExportEmptyIndices()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.path() + "/test_empty.csv";

    auto entries = makeEntries();
    bool ok = DltExport::exportToCsv(path, {}, {}, "", entries);
    QVERIFY(!ok);
}

void TestDltExport::testExportEmptyEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.path() + "/test_empty_all.csv";

    bool ok = DltExport::exportAllEntries(path, {});
    QVERIFY(!ok);
}
