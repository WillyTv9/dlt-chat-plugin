#include "test_categoryregistry.h"
#include "dltchat/category_registry.h"
#include "dltchat/analyzer_interface.h"

#include <QTest>

using namespace dltchat;

void TestCategoryRegistry::testRegistryLoads()
{
    auto &reg = CategoryRegistry::instance();
    QVERIFY(reg.categories().size() >= 20);
    QVERIFY(reg.quickActions().size() >= 30);
    QVERIFY(!reg.allCompletionStrings().isEmpty());
}

void TestCategoryRegistry::testResolveGpsAlias()
{
    auto resolved = CategoryRegistry::instance().resolveQuery("gps");
    QCOMPARE(resolved.kind, ResolvedQuery::Kind::Category);
    QCOMPARE(resolved.categoryId, QStringLiteral("GPS_NAVIGATION"));
}

void TestCategoryRegistry::testResolveCombinedGpsErrors()
{
    auto resolved = CategoryRegistry::instance().resolveQuery("gps_errors");
    QCOMPARE(resolved.kind, ResolvedQuery::Kind::CombinedFilter);
    QCOMPARE(resolved.combinedId, QStringLiteral("gps_errors"));
    QVERIFY(resolved.levelFilter.contains(QStringLiteral("error")));
}

void TestCategoryRegistry::testClassifyCarPlay()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = QStringLiteral("com.apple.carplay");
    e.payload = QStringLiteral("iAP2 handshake");
    e.level = QStringLiteral("info");
    CategoryRegistry::instance().classifyEntry(e);
    QCOMPARE(e.domain, QStringLiteral("carplay"));
    QCOMPARE(e.category, QStringLiteral("APPLE_CARPLAY"));
}

void TestCategoryRegistry::testClassifyAndroidAuto()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = QStringLiteral("carappservice");
    e.payload = QStringLiteral("AOA connected");
    e.level = QStringLiteral("info");
    CategoryRegistry::instance().classifyEntry(e);
    QCOMPARE(e.domain, QStringLiteral("androidauto"));
    QCOMPARE(e.category, QStringLiteral("ANDROID_AUTO"));
}

void TestCategoryRegistry::testFilterUsesAllKeywords()
{
  DltAnalyzerInterface::LogEntry e;
  e.index = 1;
  e.level = QStringLiteral("info");
  e.payload = QStringLiteral("DAB ensemble scan complete");
  e.apid = QStringLiteral("RADIO");
  CategoryRegistry::instance().classifyEntry(e);

  auto resolved = CategoryRegistry::instance().resolveQuery("dab");
  auto filtered = CategoryRegistry::instance().filterEntries({e}, resolved);
  QCOMPARE(filtered.size(), 1);

  DltAnalyzerInterface::LogEntry e2;
  e2.index = 2;
  e2.level = QStringLiteral("info");
  e2.payload = QStringLiteral("unrelated message");
  e2.apid = QStringLiteral("TEST");
  auto filtered2 = CategoryRegistry::instance().filterEntries({e, e2}, resolved);
  QCOMPARE(filtered2.size(), 1);
}

void TestCategoryRegistry::testLevelOnlyError()
{
    DltAnalyzerInterface::LogEntry e;
    e.index = 5;
    e.level = QStringLiteral("fatal");
    e.payload = QStringLiteral("critical failure");
    auto resolved = CategoryRegistry::instance().resolveQuery("error");
    auto filtered = CategoryRegistry::instance().filterEntries({e}, resolved);
    QCOMPARE(filtered.size(), 1);
}

void TestCategoryRegistry::testProjectionAuthErrors()
{
    DltAnalyzerInterface::LogEntry e;
    e.index = 7;
    e.level = QStringLiteral("error");
    e.domain = QStringLiteral("carplay");
    e.payload = QStringLiteral("TLS certificate validation failed");
    CategoryRegistry::instance().classifyEntry(e);

    auto resolved = CategoryRegistry::instance().resolveQuery("auth_errors");
    auto filtered = CategoryRegistry::instance().filterEntries({e}, resolved);
    QVERIFY(filtered.size() >= 1);
}
