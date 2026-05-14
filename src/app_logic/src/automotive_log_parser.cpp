#include "dltchat/automotive_log_parser.h"

#include <algorithm>
#include <QJsonArray>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

namespace dltchat {

static bool payloadContains(const DltAnalyzerInterface::LogEntry &e, const QString &keyword)
{
    return e.payload.contains(keyword, Qt::CaseInsensitive)
        || e.apid.contains(keyword, Qt::CaseInsensitive)
        || e.ctid.contains(keyword, Qt::CaseInsensitive);
}

void AutomotiveLogParser::classify(DltAnalyzerInterface::LogEntry &entry)
{
    // CarPlay detection
    if (entry.apid.contains("carplay", Qt::CaseInsensitive) ||
        entry.apid.contains("apple", Qt::CaseInsensitive) ||
        payloadContains(entry, "iap2") ||
        payloadContains(entry, "airplay"))
    {
        entry.domain = "carplay";
        if (payloadContains(entry, "video") && payloadContains(entry, "focus"))
            entry.event = "video_focus_lost";
        else if (payloadContains(entry, "audio") && payloadContains(entry, "duck"))
            entry.event = "audio_ducking";
        else if (payloadContains(entry, "mdns") || payloadContains(entry, "dns-sd"))
            entry.event = "mdns_handshake";
        else if (payloadContains(entry, "hid"))
            entry.event = "hid_event";
        else if (payloadContains(entry, "tls") || payloadContains(entry, "auth"))
            entry.event = "auth_tls";
        return;
    }

    // Android Auto detection
    if (entry.apid.contains("carappservice", Qt::CaseInsensitive) ||
        entry.apid.contains("aoap", Qt::CaseInsensitive) ||
        entry.apid.contains("androidauto", Qt::CaseInsensitive) ||
        payloadContains(entry, "usb_accessory") ||
        payloadContains(entry, "aoa"))
    {
        entry.domain = "androidauto";
        if (payloadContains(entry, "sensor"))
            entry.event = "sensor_data";
        else if (payloadContains(entry, "audio") && payloadContains(entry, "focus"))
            entry.event = "audio_focus";
        else if (payloadContains(entry, "mdns"))
            entry.event = "mdns_handshake";
        else if (payloadContains(entry, "session") || payloadContains(entry, "start"))
            entry.event = "session_start";
        else if (payloadContains(entry, "session") || payloadContains(entry, "stop"))
            entry.event = "session_stop";
        return;
    }

    entry.domain = "generic";
    entry.event.clear();
}

QVector<DltAnalyzerInterface::LogEntry> AutomotiveLogParser::filterByPreset(
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QString &presetName)
{
    QVector<DltAnalyzerInterface::LogEntry> results;

    for (const auto &e : entries)
    {
        if (presetName == "carplay" && e.domain == "carplay")
            results.append(e);
        else if (presetName == "androidauto" && e.domain == "androidauto")
            results.append(e);
        else if (presetName == "video_focus" && e.event == "video_focus_lost")
            results.append(e);
        else if (presetName == "audio_ducking" && e.event == "audio_ducking")
            results.append(e);
        else if (presetName == "mdns" && e.event == "mdns_handshake")
            results.append(e);
        else if (presetName == "sensor_data" && e.event == "sensor_data")
            results.append(e);
        else if (presetName == "auth_errors" && e.event.contains("auth", Qt::CaseInsensitive))
            results.append(e);
        else if (presetName == "session" && (e.event == "session_start" || e.event == "session_stop"))
            results.append(e);
    }

    if (presetName == "carplay" && results.isEmpty()) {
        QRegularExpression rx("carplay", QRegularExpression::CaseInsensitiveOption);
        for (const auto &e : entries) {
            if (e.payload.contains(rx) ||
                e.apid.contains(rx) ||
                e.ctid.contains(rx) ||
                e.domain.contains(rx)) {
                results.append(e);
            }
        }
    }

    std::sort(results.begin(), results.end(), [](const DltAnalyzerInterface::LogEntry &a, const DltAnalyzerInterface::LogEntry &b) {
        return a.index < b.index;
    });
    return results;
}

QPair<int, int> AutomotiveLogParser::domainStats(const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    int carplay = 0, androidauto = 0;
    for (const auto &e : entries)
    {
        if (e.domain == "carplay") carplay++;
        else if (e.domain == "androidauto") androidauto++;
    }
    return {carplay, androidauto};
}

QHash<QString, QStringList> AutomotiveLogParser::availablePresets()
{
    QHash<QString, QStringList> presets;
    presets["carplay"] = {"carplay"};
    presets["androidauto"] = {"androidauto"};
    presets["video_focus"] = {"video_focus"};
    presets["audio_ducking"] = {"audio_ducking"};
    presets["mdns"] = {"mdns"};
    presets["sensor_data"] = {"sensor_data"};
    presets["auth_errors"] = {"auth_errors"};
    presets["session"] = {"session"};
    return presets;
}

} // namespace dltchat
