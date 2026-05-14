#include "test_automotivelogparser.h"
#include "dltchat/automotive_log_parser.h"
#include <QTest>
#include <QPair>

using namespace dltchat;

void TestAutomotiveLogParser::testClassifyCarPlayByApid()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "com.apple.carplay";
    e.ctid = "CTX";
    e.payload = "some carplay data";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "carplay");
}

void TestAutomotiveLogParser::testClassifyCarPlayByPayload()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "APP1";
    e.ctid = "CTX";
    e.payload = "iap2 data transfer completed";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "carplay");
}

void TestAutomotiveLogParser::testClassifyAndroidAutoByApid()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "CarAppService";
    e.ctid = "CTX";
    e.payload = "normal payload";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "androidauto");
}

void TestAutomotiveLogParser::testClassifyAndroidAutoByPayload()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "APP";
    e.ctid = "CTX";
    e.payload = "USB_ACCESSORY connected";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "androidauto");
}

void TestAutomotiveLogParser::testClassifyGeneric()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "APP";
    e.ctid = "CTX";
    e.payload = "some random log message";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "generic");
}

void TestAutomotiveLogParser::testCarPlayEventVideoFocusLost()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "com.apple.carplay";
    e.ctid = "CTX";
    e.payload = "Video Focus session lost due to navigation";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "carplay");
    QCOMPARE(e.event, "video_focus_lost");
}

void TestAutomotiveLogParser::testCarPlayEventAudioDucking()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "com.apple.carplay";
    e.ctid = "CTX";
    e.payload = "Audio Ducking activated by incoming call";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "carplay");
    QCOMPARE(e.event, "audio_ducking");
}

void TestAutomotiveLogParser::testCarPlayEventMdns()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "APP";
    e.ctid = "CTX";
    e.payload = "_carplay._tcp service resolved";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "carplay");
    QCOMPARE(e.event, "mdns_handshake");
}

void TestAutomotiveLogParser::testAndroidAutoEventSensorData()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "CarAppService";
    e.ctid = "CTX";
    e.payload = "sensorService VehicleSpeed=65 km/h";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "androidauto");
    QCOMPARE(e.event, "sensor_data");
}

void TestAutomotiveLogParser::testAndroidAutoEventAudioFocus()
{
    DltAnalyzerInterface::LogEntry e;
    e.apid = "CarAppService";
    e.ctid = "CTX";
    e.payload = "AudioFocus request granted for MediaSession";
    AutomotiveLogParser::classify(e);
    QCOMPARE(e.domain, "androidauto");
    QCOMPARE(e.event, "audio_focus");
}

void TestAutomotiveLogParser::testFilterByPresetCarPlay()
{
    QVector<DltAnalyzerInterface::LogEntry> entries;

    DltAnalyzerInterface::LogEntry e1;
    e1.index = 0; e1.domain = "carplay"; e1.event = ""; e1.level = "info";
    entries.append(e1);

    DltAnalyzerInterface::LogEntry e2;
    e2.index = 1; e2.domain = "androidauto"; e2.event = ""; e2.level = "info";
    entries.append(e2);

    DltAnalyzerInterface::LogEntry e3;
    e3.index = 2; e3.domain = "generic"; e3.event = ""; e3.level = "info";
    entries.append(e3);

    auto filtered = AutomotiveLogParser::filterByPreset(entries, "carplay");
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered[0].index, 0);
}

void TestAutomotiveLogParser::testFilterByPresetAuthErrors()
{
    QVector<DltAnalyzerInterface::LogEntry> entries;

    DltAnalyzerInterface::LogEntry e1;
    e1.index = 10; e1.domain = "carplay"; e1.level = "error";
    entries.append(e1);

    DltAnalyzerInterface::LogEntry e2;
    e2.index = 11; e2.domain = "androidauto"; e2.level = "error";
    entries.append(e2);

    DltAnalyzerInterface::LogEntry e3;
    e3.index = 12; e3.domain = "carplay"; e3.level = "info";
    entries.append(e3);

    auto filtered = AutomotiveLogParser::filterByPreset(entries, "auth_errors");
    QCOMPARE(filtered.size(), 2);
}

void TestAutomotiveLogParser::testAvailablePresets()
{
    QHash<QString, QStringList> presets = AutomotiveLogParser::availablePresets();
    QVERIFY(presets.contains("carplay"));
    QVERIFY(presets.contains("androidauto"));
    QVERIFY(presets.contains("video_focus"));
    QVERIFY(presets.contains("audio_ducking"));
    QVERIFY(presets.contains("mdns"));
    QVERIFY(presets.contains("sensor_data"));
    QVERIFY(presets.contains("auth_errors"));
    QVERIFY(presets.contains("session"));
}

void TestAutomotiveLogParser::testDomainStats()
{
    QVector<DltAnalyzerInterface::LogEntry> entries;

    DltAnalyzerInterface::LogEntry e1;
    e1.domain = "carplay"; entries.append(e1);

    DltAnalyzerInterface::LogEntry e2;
    e2.domain = "carplay"; entries.append(e2);

    DltAnalyzerInterface::LogEntry e3;
    e3.domain = "androidauto"; entries.append(e3);

    DltAnalyzerInterface::LogEntry e4;
    e4.domain = "generic"; entries.append(e4);

    auto stats = AutomotiveLogParser::domainStats(entries);
    QCOMPARE(stats.first, 2);  // carplay
    QCOMPARE(stats.second, 1); // androidauto
}
