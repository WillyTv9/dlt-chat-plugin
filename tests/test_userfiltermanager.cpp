#include "test_userfiltermanager.h"
#include "dltchat/user_filter_manager.h"
#include <QTest>

using namespace dltchat;
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QString TestUserFilterManager::createSampleFilterFile(const QTemporaryDir &dir) const
{
    QString path = dir.path() + "/filters.json";

    QJsonObject root;
    root["version"] = "1.0.0";
    root["description"] = "Test filters";

    QJsonArray filters;

    QJsonObject f1;
    f1["label"] = "CAN Errors";
    f1["pattern"] = "CAN.*error";
    f1["fields"] = QJsonArray{"payload"};
    f1["color"] = "#FF0000";
    f1["level"] = QJsonArray{"error", "fatal"};
    f1["enabled"] = true;
    filters.append(f1);

    QJsonObject f2;
    f2["label"] = "CarPlay Events";
    f2["pattern"] = "carplay|iap2";
    f2["fields"] = QJsonArray{"payload", "apid"};
    f2["color"] = "#00AAFF";
    f2["domain"] = "carplay";
    f2["enabled"] = true;
    filters.append(f2);

    root["filters"] = filters;

    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(root).toJson());
    file.close();
    return path;
}

void TestUserFilterManager::testLoadValidFilters()
{
    UserFilterManager mgr;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = createSampleFilterFile(dir);

    QString error;
    bool ok = mgr.loadFromFile(path, &error);
    QVERIFY(ok);
    QCOMPARE(mgr.filters().size(), 2);
    QCOMPARE(mgr.activeFilterCount(), 2);
}

void TestUserFilterManager::testLoadInvalidFile()
{
    UserFilterManager mgr;
    QString error;
    bool ok = mgr.loadFromFile("/nonexistent/path/filters.json", &error);
    QVERIFY(!ok);
    QVERIFY(!error.isEmpty());
}

void TestUserFilterManager::testLoadMissingVersion()
{
    UserFilterManager mgr;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.path() + "/bad_filters.json";

    QJsonObject root;
    QJsonArray filters;
    root["filters"] = filters;

    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(root).toJson());
    file.close();

    QString error;
    bool ok = mgr.loadFromFile(path, &error);
    QVERIFY(!ok);
}

void TestUserFilterManager::testActiveFilterCount()
{
    UserFilterManager mgr;
    QCOMPARE(mgr.activeFilterCount(), 0);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    mgr.loadFromFile(createSampleFilterFile(dir));
    QCOMPARE(mgr.activeFilterCount(), 2);
}

void TestUserFilterManager::testApplyToEntries()
{
    UserFilterManager mgr;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    mgr.loadFromFile(createSampleFilterFile(dir));

    QVector<DltAnalyzerInterface::LogEntry> entries;
    DltAnalyzerInterface::LogEntry e;
    e.index = 10; e.apid = "APP"; e.ctid = "CTX"; e.level = "error";
    e.domain = "generic"; e.payload = "CAN bus error: arbitration lost";
    entries.append(e);

    QHash<int, QColor> highlights = mgr.applyToEntries(entries);
    QVERIFY(highlights.contains(10));
}

void TestUserFilterManager::testClear()
{
    UserFilterManager mgr;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    mgr.loadFromFile(createSampleFilterFile(dir));
    QCOMPARE(mgr.filters().size(), 2);

    mgr.clearFilters();
    QCOMPARE(mgr.filters().size(), 0);
    QCOMPARE(mgr.activeFilterCount(), 0);
}

void TestUserFilterManager::testMatchesEntryByDomain()
{
    UserFilterManager mgr;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString path = dir.path() + "/domain_filter.json";
    QJsonObject f;
    f["label"] = "CP Test";
    f["pattern"] = "test";
    f["fields"] = QJsonArray{"payload"};
    f["color"] = "#FF0000";
    f["domain"] = "carplay";
    f["enabled"] = true;

    QJsonObject root;
    root["version"] = "1.0.0";
    root["filters"] = QJsonArray{f};

    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(root).toJson());
    file.close();

    mgr.loadFromFile(path);

    DltAnalyzerInterface::LogEntry e;
    e.index = 5; e.apid = "APP"; e.ctid = "CTX"; e.level = "info";
    e.payload = "test entry";
    e.domain = "generic";

    QVector<DltAnalyzerInterface::LogEntry> entries = {e};
    QHash<int, QColor> highlights = mgr.applyToEntries(entries);
    QVERIFY(!highlights.contains(5)); // domain doesn't match

    e.domain = "carplay";
    entries = {e};
    highlights = mgr.applyToEntries(entries);
    QVERIFY(highlights.contains(5)); // now domain matches
}

void TestUserFilterManager::testMatchesEntryByLevel()
{
    UserFilterManager mgr;
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString path = dir.path() + "/level_filter.json";
    QJsonObject f;
    f["label"] = "Error Filter";
    f["pattern"] = "crash";
    f["fields"] = QJsonArray{"payload"};
    f["color"] = "#FF0000";
    f["level"] = QJsonArray{"error", "fatal"};
    f["enabled"] = true;

    QJsonObject root;
    root["version"] = "1.0.0";
    root["filters"] = QJsonArray{f};

    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(root).toJson());
    file.close();

    mgr.loadFromFile(path);

    DltAnalyzerInterface::LogEntry e;
    e.index = 7; e.apid = "APP"; e.ctid = "CTX"; e.level = "info";
    e.payload = "system crash detected";

    QVector<DltAnalyzerInterface::LogEntry> entries = {e};
    QHash<int, QColor> highlights = mgr.applyToEntries(entries);
    QVERIFY(!highlights.contains(7)); // level doesn't match

    e.level = "error";
    entries = {e};
    highlights = mgr.applyToEntries(entries);
    QVERIFY(highlights.contains(7)); // now level matches
}
