#include "automotivelogparser.h"

const AutomotiveLogParser::PresetRules &AutomotiveLogParser::presetRules()
{
    static PresetRules rules;
    static bool init = false;
    if (!init) {
        init = true;

        rules["carplay"] = {
            {"domain", QRegularExpression("^carplay$", QRegularExpression::CaseInsensitiveOption)}
        };
        rules["androidauto"] = {
            {"domain", QRegularExpression("^androidauto$", QRegularExpression::CaseInsensitiveOption)}
        };
        rules["video_focus"] = {
            {"event", QRegularExpression("^video_focus_lost$")}
        };
        rules["audio_ducking"] = {
            {"event", QRegularExpression("^audio_ducking$")}
        };
        rules["mdns"] = {
            {"event", QRegularExpression("^mdns_handshake$")}
        };
        rules["sensor_data"] = {
            {"event", QRegularExpression("^sensor_data$")}
        };
        rules["auth_errors"] = {
            {"domain", QRegularExpression("^(carplay|androidauto)$")},
            {"level",  QRegularExpression("^(error|fatal)$")}
        };
        rules["session"] = {
            {"event", QRegularExpression("^(session_start|session_stop)$")}
        };
    }
    return rules;
}

void AutomotiveLogParser::classify(DltAnalyzerInterface::LogEntry &entry)
{
    entry.domain = QStringLiteral("generic");
    entry.event.clear();

    const QString &apid = entry.apid;
    const QString &ctid = entry.ctid;
    const QString &payload = entry.payload;

    bool isCarPlay = false;

    if (apid.contains("com.apple.carplay", Qt::CaseInsensitive) ||
        ctid.contains("com.apple.carplay", Qt::CaseInsensitive))
        isCarPlay = true;

    if (!isCarPlay && (payload.contains("iap2", Qt::CaseInsensitive) ||
                       payload.contains("AirPlay") ||
                       payload.contains("_carplay._tcp", Qt::CaseInsensitive)))
        isCarPlay = true;

    if (isCarPlay) {
        entry.domain = QStringLiteral("carplay");

        if (payload.contains("Video Focus", Qt::CaseInsensitive) &&
            payload.contains("lost", Qt::CaseInsensitive))
            entry.event = QStringLiteral("video_focus_lost");
        else if (payload.contains("Audio", Qt::CaseInsensitive) &&
                 payload.contains("Duck", Qt::CaseInsensitive))
            entry.event = QStringLiteral("audio_ducking");
        else if (payload.contains("_carplay._tcp", Qt::CaseInsensitive))
            entry.event = QStringLiteral("mdns_handshake");
        else if (payload.contains("HIDReport", Qt::CaseInsensitive) ||
                 (payload.contains("iap2", Qt::CaseInsensitive) &&
                  payload.contains("hid", Qt::CaseInsensitive)))
            entry.event = QStringLiteral("hid_event");
        else if (payload.contains("certificate", Qt::CaseInsensitive) ||
                 payload.contains("TLS"))
            entry.event = QStringLiteral("auth_tls");
        else if ((payload.contains("session", Qt::CaseInsensitive) &&
                  (payload.contains("stop", Qt::CaseInsensitive) ||
                   payload.contains("disconnect", Qt::CaseInsensitive))) ||
                 payload.contains("link down", Qt::CaseInsensitive))
            entry.event = QStringLiteral("session_stop");

        return;
    }

    bool isAndroidAuto = false;

    if (apid.contains("CarAppService", Qt::CaseInsensitive) ||
        apid.contains("AOAP", Qt::CaseInsensitive) ||
        apid.contains("AndroidAuto", Qt::CaseInsensitive) ||
        ctid.contains("CarAppService", Qt::CaseInsensitive) ||
        ctid.contains("AOAP", Qt::CaseInsensitive) ||
        ctid.contains("AndroidAuto", Qt::CaseInsensitive))
        isAndroidAuto = true;

    if (!isAndroidAuto &&
        (payload.contains("USB_ACCESSORY", Qt::CaseInsensitive) ||
         payload.contains("AOA", Qt::CaseInsensitive) ||
         payload.contains("androidauto", Qt::CaseInsensitive)))
        isAndroidAuto = true;

    if (isAndroidAuto) {
        entry.domain = QStringLiteral("androidauto");

        if (payload.contains("sensorService", Qt::CaseInsensitive) ||
            payload.contains("VehicleSpeed", Qt::CaseInsensitive) ||
            payload.contains("sensorEvent", Qt::CaseInsensitive))
            entry.event = QStringLiteral("sensor_data");
        else if (payload.contains("MediaSession", Qt::CaseInsensitive) ||
                 payload.contains("AudioFocus", Qt::CaseInsensitive))
            entry.event = QStringLiteral("audio_focus");
        else if (payload.contains("_androidauto._tcp", Qt::CaseInsensitive))
            entry.event = QStringLiteral("mdns_handshake");
        else if (payload.contains("projection", Qt::CaseInsensitive) &&
                 (payload.contains("start", Qt::CaseInsensitive) ||
                  payload.contains("session", Qt::CaseInsensitive)))
            entry.event = QStringLiteral("session_start");
        else if (payload.contains("projection", Qt::CaseInsensitive) &&
                 (payload.contains("stop", Qt::CaseInsensitive) ||
                  payload.contains("session", Qt::CaseInsensitive)))
            entry.event = QStringLiteral("session_stop");

        return;
    }
}

QVector<DltAnalyzerInterface::LogEntry> AutomotiveLogParser::filterByPreset(
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QString &presetName, int maxResults)
{
    auto it = presetRules().constFind(presetName);
    if (it == presetRules().constEnd())
        return {};

    const auto &rules = it.value();
    QVector<DltAnalyzerInterface::LogEntry> result;
    result.reserve(qMin(entries.size(), maxResults));

    for (const auto &e : entries) {
        bool match = true;
        for (const auto &rule : rules) {
            const QString &field = rule.first;
            const QRegularExpression &re = rule.second;

            QString value;
            if (field == "domain") value = e.domain;
            else if (field == "event") value = e.event;
            else if (field == "level") value = e.level;
            else if (field == "apid") value = e.apid;
            else if (field == "ctid") value = e.ctid;
            else if (field == "payload") value = e.payload;

            if (!re.match(value).hasMatch()) {
                match = false;
                break;
            }
        }
        if (match) {
            result.append(e);
            if (result.size() >= maxResults) break;
        }
    }
    return result;
}

QStringList AutomotiveLogParser::availablePresets()
{
    return presetRules().keys();
}

QPair<int, int> AutomotiveLogParser::domainStats(
    const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    int cp = 0, aa = 0;
    for (const auto &e : entries) {
        if (e.domain == "carplay") cp++;
        else if (e.domain == "androidauto") aa++;
    }
    return {cp, aa};
}
