#include "test_rulebasedanalyzer.h"
#include <QTest>

using namespace dltchat;

QVector<DltAnalyzerInterface::LogEntry> TestRuleBasedAnalyzer::makeSampleEntries() const
{
    QVector<DltAnalyzerInterface::LogEntry> entries;
    for (int i = 0; i < 10; ++i) {
        DltAnalyzerInterface::LogEntry e;
        e.index = i;
        e.time = QString("2026-05-12 10:00:%1.000000").arg(i, 2, 10, QLatin1Char('0'));
        e.timestamp = QString("123456%1").arg(i);
        e.ecu = "ECU1";
        e.apid = "APP1";
        e.ctid = "CTX1";
        e.payload = QString("Log message number %1").arg(i);
        e.domain = "generic";
        if (i == 0) { e.level = "fatal"; e.payload = "Fatal: system crash at module X"; }
        else if (i == 1) { e.level = "error"; e.payload = "Error: timeout waiting for response from ECU_XYZ"; }
        else if (i == 2) { e.level = "warn"; e.payload = "Warning: memory usage high (85%)"; }
        else if (i == 3) { e.level = "info"; e.payload = "Info: service started successfully"; }
        else if (i == 4) { e.level = "debug"; e.payload = "Debug: CAN message ID 0x123 received"; }
        else if (i == 5) { e.level = "error"; e.payload = "Error: CAN bus communication failure"; }
        else if (i == 6) { e.level = "warn"; e.payload = "Warning: authentication token expired"; }
        else if (i == 7) { e.level = "info"; e.payload = "Info: GPS position update lat=45.0 lon=9.0"; }
        else if (i == 8) { e.level = "debug"; e.payload = "Debug: memory allocation 1024 bytes"; }
        else { e.level = "verbose"; e.payload = "Verbose: heartbeat signal OK"; }
        entries.append(e);
    }
    return entries;
}

QVector<DltAnalyzerInterface::LogEntry> TestRuleBasedAnalyzer::makeAutoEntries() const
{
    QVector<DltAnalyzerInterface::LogEntry> entries;
    entries.reserve(6);

    DltAnalyzerInterface::LogEntry e1;
    e1.index = 100; e1.level = "info"; e1.apid = "com.apple.carplay"; e1.ctid = "CTX";
    e1.payload = "CarPlay session started with Video Focus"; e1.domain = "carplay";
    e1.event = "video_focus_lost";
    entries.append(e1);

    DltAnalyzerInterface::LogEntry e2;
    e2.index = 101; e2.level = "info"; e2.apid = "APP"; e2.ctid = "CTX";
    e2.payload = "Audio Ducking activated by phone call"; e2.domain = "carplay";
    e2.event = "audio_ducking";
    entries.append(e2);

    DltAnalyzerInterface::LogEntry e3;
    e3.index = 102; e3.level = "info"; e3.apid = "CarAppService"; e3.ctid = "CTX";
    e3.payload = "Android Auto projection session started"; e3.domain = "androidauto";
    e3.event = "session_start";
    entries.append(e3);

    DltAnalyzerInterface::LogEntry e4;
    e4.index = 103; e4.level = "error"; e4.apid = "APP"; e4.ctid = "CTX";
    e4.payload = "AOA: USB accessory disconnected unexpectedly"; e4.domain = "androidauto";
    e4.event = "";
    entries.append(e4);

    DltAnalyzerInterface::LogEntry e5;
    e5.index = 104; e5.level = "warn"; e5.apid = "APP"; e5.ctid = "CTX";
    e5.payload = "_androidauto._tcp handshake timeout"; e5.domain = "androidauto";
    e5.event = "mdns_handshake";
    entries.append(e5);

    DltAnalyzerInterface::LogEntry e6;
    e6.index = 105; e6.level = "info"; e6.apid = "APP"; e6.ctid = "CTX";
    e6.payload = "VehicleSpeed sensor data: 65 km/h"; e6.domain = "androidauto";
    e6.event = "sensor_data";
    entries.append(e6);

    return entries;
}

void TestRuleBasedAnalyzer::testEmptyEntries()
{
    DltRuleBasedAnalyzer analyzer;
    auto result = analyzer.analyzeQuery("error", {});
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Nessun log"));
}

void TestRuleBasedAnalyzer::testEmptyQuery()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Scrivi una parola chiave"));
}

void TestRuleBasedAnalyzer::testHelpCommand()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("help", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Azioni Rapide"));
    QVERIFY(result.indices.isEmpty());

    result = analyzer.analyzeQuery("aiuto", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Azioni Rapide"));

    result = analyzer.analyzeQuery("comandi", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Azioni Rapide"));
}

void TestRuleBasedAnalyzer::testTimelineCommand()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("timeline", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Sequenza completa"));
    QCOMPARE(result.indices.size(), entries.size());

    result = analyzer.analyzeQuery("cronologia", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Sequenza completa"));
}

void TestRuleBasedAnalyzer::testKeywordsCommand()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("keywords", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Categorie riconosciute"));

    result = analyzer.analyzeQuery("categorie", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Categorie riconosciute"));
}

void TestRuleBasedAnalyzer::testSummaryCommand()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("summary", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Riepilogo log"));
    QVERIFY(result.responseHtml.contains("10 messaggi totali"));

    result = analyzer.analyzeQuery("riassumi", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Riepilogo log"));

    result = analyzer.analyzeQuery("statistiche", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("Riepilogo log"));
}

void TestRuleBasedAnalyzer::testPatternDetection()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    entries.append(entries[4]); // duplicate index 4
    entries.last().index = 20; // but different index
    entries.append(entries[5]); // duplicate index 5
    entries.last().index = 21;

    auto result = analyzer.analyzeQuery("pattern", entries);
    QVERIFY(result.success);
    QVERIFY(result.responseHtml.contains("pattern"));
}

void TestRuleBasedAnalyzer::testLevelFilterError()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("error", entries);
    QVERIFY(result.success);
    QVERIFY(!result.indices.isEmpty());
    for (int idx : result.indices) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [idx](const auto &e) { return e.index == idx; });
        QVERIFY(it != entries.end());
        QVERIFY(it->level == "error" || it->level == "fatal");
    }
}

void TestRuleBasedAnalyzer::testLevelFilterWarn()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("warn", entries);
    QVERIFY(result.success);
    QVERIFY(!result.indices.isEmpty());
    for (int idx : result.indices) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [idx](const auto &e) { return e.index == idx; });
        QVERIFY(it != entries.end());
        QCOMPARE(it->level, "warn");
    }
}

void TestRuleBasedAnalyzer::testLevelFilterInfo()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("info", entries);
    QVERIFY(result.success);
    QVERIFY(!result.indices.isEmpty());
    for (int idx : result.indices) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [idx](const auto &e) { return e.index == idx; });
        QVERIFY(it != entries.end());
        QCOMPARE(it->level, "info");
    }
}

void TestRuleBasedAnalyzer::testLevelFilterDebug()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("debug", entries);
    QVERIFY(result.success);
    QVERIFY(!result.indices.isEmpty());
    for (int idx : result.indices) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [idx](const auto &e) { return e.index == idx; });
        QVERIFY(it != entries.end());
        QCOMPARE(it->level, "debug");
    }
}

void TestRuleBasedAnalyzer::testDomainFilterCarPlay()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeAutoEntries();
    auto result = analyzer.analyzeQuery("carplay info", entries);
    QVERIFY(result.success);
    QVERIFY(!result.indices.isEmpty());
    for (int idx : result.indices) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [idx](const auto &e) { return e.index == idx; });
        QVERIFY(it != entries.end());
        QCOMPARE(it->domain, "carplay");
    }
}

void TestRuleBasedAnalyzer::testDomainFilterAndroidAuto()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeAutoEntries();
    auto result = analyzer.analyzeQuery("androidauto", entries);
    QVERIFY(result.success);
    QVERIFY(!result.indices.isEmpty());
    for (int idx : result.indices) {
        auto it = std::find_if(entries.begin(), entries.end(),
            [idx](const auto &e) { return e.index == idx; });
        QVERIFY(it != entries.end());
        QCOMPARE(it->domain, "androidauto");
    }
}

void TestRuleBasedAnalyzer::testCombinedFilter()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("error can", entries);
    QVERIFY(result.success);
}

void TestRuleBasedAnalyzer::testNoMatch()
{
    DltRuleBasedAnalyzer analyzer;
    auto entries = makeSampleEntries();
    auto result = analyzer.analyzeQuery("zzz_nonexistent_zzz", entries);
    QVERIFY(result.success);
    QVERIFY(result.indices.isEmpty());
    QVERIFY(result.responseHtml.contains("Nessun messaggio"));
}

void TestRuleBasedAnalyzer::testSimplifyPayload()
{
    QString result = DltRuleBasedAnalyzer::simplifyPayload("test\nmessage\r\nwith\0null");
    QVERIFY(!result.contains(QChar::Null));
    QVERIFY(!result.contains('\n'));

    QString withPassword = "PASSWORD=supersecret123";
    QString sanitized = DltRuleBasedAnalyzer::simplifyPayload(withPassword);
    QVERIFY(!sanitized.contains("supersecret123"));
    QVERIFY(sanitized.contains("***"));
}

void TestRuleBasedAnalyzer::testFormatEntryLine()
{
    DltAnalyzerInterface::LogEntry e;
    e.index = 42; e.time = "10:00:00.123"; e.level = "error";
    e.apid = "APP1"; e.ctid = "CTX1"; e.payload = "test payload";
    QString line = DltRuleBasedAnalyzer::formatEntryLine(e);
    QVERIFY(line.contains("[42]"));
    QVERIFY(line.contains("ERROR"));
    QVERIFY(line.contains("APP1/CTX1"));
    QVERIFY(line.contains("test payload"));
}

void TestRuleBasedAnalyzer::testConfigurationInfo()
{
    DltRuleBasedAnalyzer analyzer;
    QString info = analyzer.configurationInfo();
    QVERIFY(!info.isEmpty());
    QVERIFY(info.contains("Rule-Based"));
}
