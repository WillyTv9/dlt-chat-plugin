#include "dltchatanalyzer.h"

#include <QRegularExpression>
#include <QSet>

#include <algorithm>

struct KeywordCategory {
    QString name;
    QStringList keywords;
    QString description;
};

static const KeywordCategory CATEGORIES[] = {
    {"Errori/Falli", {"error", "fail", "failure", "fault", "exception", "crash", "panic", "abort", "fatal", "critical", "errore", "err", "failed", "ko"}, "Errori e fallimenti di sistema"},
    {"Comunicazione", {"can", "ethernet", "tcp", "udp", "ip", "http", "websocket", "mqtt", "bluetooth", "wifi", "serial", "uart", "spi", "i2c", "lin", "flexray", "most", " connection", "disconnect", "timeout"}, "Comunicazione di rete e bus"},
    {"Timing", {"latency", "delay", "slow", "performance", "response", "elapsed", "duration", "timeout", "wait", " hang", "stuck", "freeze", "lento", "ritardo"}, "Problematiche di timing e performance"},
    {"Memoria", {"memory", "heap", "stack", "alloc", "free", "leak", "overflow", "underflow", "buffer", "null", "pointer", "segfault", "corruption", "memoria"}, "Problemi di memoria"},
    {"ECU/Sistemi", {"ecu", "ecus", "gateway", "sensor", "actuator", "controller", "module", "unit", "node", "device", "subsystem", "component"}, "Componenti e sistemi ECU"},
    {"Stato", {"init", "start", "stop", "restart", "shutdown", "sleep", "wake", "suspend", "resume", "boot", "reset", "enable", "disable", "state"}, "Stati di sistema e transizioni"},
    {"Dati/Payload", {"payload", "data", "frame", "packet", "message", "signal", "value", "invalid", "malformed", "parse", "decode", "encode"}, "Gestione dati e payload"},
    {"Security", {"auth", "authentication", "authorization", "permission", "denied", "unauthorized", "access", "security", "certificate", "encryption", "crypto", "token", "login", "logout"}, "Sicurezza e autenticazione"},
    {"Veicolo", {"vehicle", "speed", "brake", "accelerator", "steering", "gear", "engine", "battery", "charging", "adcu", "hvac", "infotainment", "telemetry"}, "Funzioni veicolo"},
    {"Diagnostica", {"diagnostic", "dtc", "obd", "oBD", "code", "faultcode", "trouble", "status", "health", "monitor", "test", "check"}, "Diagnostica e codici errore"},
    {"Network", {"socket", "port", "host", "address", "route", "switch", "router", "firewall", "dns", "dhcp", "arp", "ping", "packet", "bandwidth"}, "Network e protocolli"},
    {"Storage", {"storage", "disk", "sd", "nand", "flash", "eMMC", "file", "filesystem", "read", "write", "mount", "umount", "capacity"}, "Storage e file system"},
    {"Processi", {"process", "thread", "task", "job", "queue", "scheduler", "priority", "cpu", "load", "deadlock", "mutex", "semaphore", "sync"}, "Processi e sincronizzazione"},
    {"Updates", {"update", "upgrade", "flash", "download", "install", "version", "firmware", "software", "ota", "package"}, "Update e upgrade software"},
    {"Video/Audio", {"video", "audio", "camera", "display", "screen", "codec", "stream", "frame", "fps", "resolution", "latency"}, "Multimedia e video"},
    {"Navigazione", {"gps", "position", "location", "map", "route", "navigation", "heading", "speed", "altitude", "satellite", "gnss"}, "Navigazione e posizione"},
    {"Temperature", {"temperature", "temp", "overheat", "thermal", "cooling", "heater", "sensor", "threshold", "celsius", "fahrenheit"}, "Gestione temperatura"},
    {"Voltaggio", {"voltage", "current", "power", "battery", "charger", "supply", "vreg", "boost", "buck", "power"}, "Gestione alimentazione"},
    {"CAN Bus", {"can", "canfd", "canfd", "identifier", "id", "dlc", "arbitation", "stuffing", "errorframe", "remote", "extended", "canid", "trc"}, "CAN bus specifico"},
    {"UDS/OBD", {"uds", "obd", "diagsession", "nrc", "negative", "response", "request", "service", "sid", "did", "rid", " routine", "diagnostic"}, "Diagnostica UDS/OBD"}
};

QStringList DltChatAnalyzer::getAllKeywords() const
{
    QStringList all;
    for (const auto &cat : CATEGORIES)
    {
        all.append(cat.keywords);
    }
    return all;
}

QStringList DltChatAnalyzer::getAllCategories() const
{
    QStringList cats;
    for (const auto &cat : CATEGORIES)
    {
        cats.append(cat.name);
    }
    return cats;
}

QStringList DltChatAnalyzer::getKeywordsForCategory(const QString &category) const
{
    for (const auto &cat : CATEGORIES)
    {
        if (cat.name.toLower() == category.toLower())
        {
            return cat.keywords;
        }
    }
    return QStringList();
}

DltChatAnalyzer::QueryResult DltChatAnalyzer::analyzeQuery(const QString &query, const QVector<LogEntry> &entries) const
{
    QueryResult result;

    if (entries.isEmpty())
    {
        result.responseHtml = "Nessun log caricato. Apri un file DLT e riprova.";
        return result;
    }

    if (query.trimmed().isEmpty())
    {
        result.responseHtml = "Inserisci una domanda o una parola chiave.";
        return result;
    }

    const QString lowerQuery = query.toLower();

    if (lowerQuery == "help" || lowerQuery == "aiuto" || lowerQuery == "?")
    {
        result.responseHtml = buildHelpHtml();
        return result;
    }

    if (lowerQuery == "keywords" || lowerQuery == "categorie")
    {
        result.responseHtml = buildCategoriesHtml();
        return result;
    }

    const bool isSummary = lowerQuery.contains("summary")
        || lowerQuery.contains("summarize")
        || lowerQuery.contains("riassumi")
        || lowerQuery.contains("sintesi")
        || lowerQuery == "statistiche"
        || lowerQuery == "stats";

    if (isSummary)
    {
        result.responseHtml = buildSummaryHtml(entries);
        return result;
    }

    const bool isListCategories = lowerQuery.contains("list") && lowerQuery.contains("category");
    if (isListCategories)
    {
        result.responseHtml = buildCategoriesHtml();
        return result;
    }

    QSet<QString> levelTokens;
    if (lowerQuery.contains("fatal") || lowerQuery.contains("fatale") || lowerQuery.contains("critico"))
    {
        levelTokens.insert("fatal");
        levelTokens.insert("error");
    }
    if (lowerQuery.contains("error") || lowerQuery.contains("errore") || lowerQuery.contains(" fail"))
    {
        levelTokens.insert("error");
    }
    if (lowerQuery.contains("warn") || lowerQuery.contains("warning") || lowerQuery.contains("avviso"))
    {
        levelTokens.insert("warn");
    }
    if (lowerQuery.contains("info"))
    {
        levelTokens.insert("info");
    }
    if (lowerQuery.contains("debug"))
    {
        levelTokens.insert("debug");
    }
    if (lowerQuery.contains("verbose"))
    {
        levelTokens.insert("verbose");
    }

    QStringList categoryKeywords;
    for (const auto &cat : CATEGORIES)
    {
        for (const QString &kw : cat.keywords)
        {
            if (lowerQuery.contains(kw))
            {
                categoryKeywords.append(kw);
            }
        }
    }

    QRegularExpression indexRegex("(index|indice|riga|line)\\s*(\\d+)");
    QRegularExpressionMatch indexMatch = indexRegex.match(lowerQuery);
    QList<int> explicitIndices;
    if (indexMatch.hasMatch())
    {
        explicitIndices.append(indexMatch.captured(2).toInt());
    }
    else
    {
        QRegularExpression numberRegex("\\b\\d+\\b");
        QRegularExpressionMatchIterator it = numberRegex.globalMatch(lowerQuery);
        while (it.hasNext())
        {
            QRegularExpressionMatch m = it.next();
            explicitIndices.append(m.captured(0).toInt());
        }
    }

    QRegularExpression tsRegex("(timestamp|time|tempo)\\s*([0-9\\.]+)");
    QRegularExpressionMatch tsMatch = tsRegex.match(lowerQuery);
    QString timestampToken;
    if (tsMatch.hasMatch())
    {
        timestampToken = tsMatch.captured(2);
    }

    const bool wantsContext = lowerQuery.contains("why")
        || lowerQuery.contains("perche")
        || lowerQuery.contains("causa")
        || lowerQuery.contains("motivo")
        || lowerQuery.contains("context")
        || lowerQuery.contains("before")
        || lowerQuery.contains("after");

    const bool wantsTimeline = lowerQuery.contains("timeline")
        || lowerQuery.contains("chronological")
        || lowerQuery.contains("ordina")
        || lowerQuery.contains("sequence");

    const bool wantsRepetition = lowerQuery.contains("repeat")
        || lowerQuery.contains("ripeti")
        || lowerQuery.contains("duplic")
        || lowerQuery.contains("pattern")
        || lowerQuery.contains("frequente");

    QSet<QString> stopwords;
    stopwords << "show" << "mostra" << "elenca" << "tutti" << "tutte" << "all"
              << "why" << "perche" << "causa" << "motivo" << "summary" << "summarize"
              << "riassumi" << "sintesi" << "log" << "logs" << "messaggi" << "messaggio"
              << "indice" << "index" << "riga" << "line" << "timestamp" << "time" << "tempo"
              << "error" << "errors" << "errore" << "errori" << "fatal" << "fatale" << "warn" << "warning"
              << "avviso" << "info" << "debug" << "verbose" << "search" << "find" << "cerca" << "trova";

    QStringList tokens = lowerQuery.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    QStringList keywords;
    for (const QString &token : tokens)
    {
        if (token.size() < 2)
        {
            continue;
        }
        if (stopwords.contains(token))
        {
            continue;
        }
        if (token.at(0).isDigit())
        {
            continue;
        }
        keywords.append(token);
    }

    if (categoryKeywords.isEmpty() && keywords.isEmpty() && levelTokens.isEmpty() && explicitIndices.isEmpty())
    {
        result.responseHtml = "Nessun criterio di ricerca. Usa 'keywords' per vedere le categorie disponibili.";
        return result;
    }

    QHash<int, LogEntry> entryByIndex;
    entryByIndex.reserve(entries.size());
    for (const LogEntry &entry : entries)
    {
        entryByIndex.insert(entry.index, entry);
    }

    QList<int> matchedIndices;
    QStringList matchedSnippets;

    auto addMatch = [&](const LogEntry &entry) {
        if (!matchedIndices.contains(entry.index))
        {
            matchedIndices.append(entry.index);
            matchedSnippets.append(entry.payload.left(120));
        }
    };

    if (!explicitIndices.isEmpty())
    {
        QStringList contextLines;
        for (int idx : explicitIndices)
        {
            if (!entryByIndex.contains(idx))
            {
                continue;
            }

            addMatch(entryByIndex.value(idx));

            for (int delta = -2; delta <= 2; ++delta)
            {
                const int neighbor = idx + delta;
                if (entryByIndex.contains(neighbor))
                {
                    contextLines.append(formatEntryLine(entryByIndex.value(neighbor)).toHtmlEscaped());
                }
            }
        }

        if (matchedIndices.isEmpty())
        {
            result.responseHtml = "Indice non trovato. Verifica i filtri o l'indice inserito.";
            return result;
        }

        result.indices = matchedIndices;
        result.snippets = matchedSnippets;
        result.responseHtml = "Ho trovato il messaggio richiesto. Contesto:\n";
        result.responseHtml += QString("<pre>%1</pre>").arg(contextLines.join("\n"));
        return result;
    }

    QVector<LogEntry> candidates;
    candidates.reserve(entries.size());

    for (const LogEntry &entry : entries)
    {
        if (!levelTokens.isEmpty() && !levelTokens.contains(entry.level))
        {
            continue;
        }
        if (!timestampToken.isEmpty())
        {
            if (!entry.time.contains(timestampToken) && !entry.timestamp.contains(timestampToken))
            {
                continue;
            }
        }

        bool hit = false;
        if (!categoryKeywords.isEmpty())
        {
            for (const QString &kw : categoryKeywords)
            {
                if (entry.payload.contains(kw, Qt::CaseInsensitive) ||
                    entry.apid.toLower().contains(kw) ||
                    entry.ctid.toLower().contains(kw))
                {
                    hit = true;
                    break;
                }
            }
        }
        else if (!keywords.isEmpty())
        {
            for (const QString &kw : keywords)
            {
                if (entry.payload.contains(kw, Qt::CaseInsensitive) ||
                    entry.apid.toLower().contains(kw) ||
                    entry.ctid.toLower().contains(kw))
                {
                    hit = true;
                    break;
                }
            }
        }
        else
        {
            hit = true;
        }

        if (!hit)
        {
            continue;
        }
        candidates.append(entry);
    }

    if (candidates.isEmpty())
    {
        result.responseHtml = "Nessun risultato. Prova con parole chiave diverse o usa 'keywords' per vedere le categorie.";
        return result;
    }

    if (wantsRepetition)
    {
        QHash<QString, int> payloadFrequency;
        for (const LogEntry &entry : candidates)
        {
            QString simplified = entry.payload.simplified();
            if (simplified.length() > 10)
            {
                payloadFrequency[simplified] += 1;
            }
        }

        QVector<QPair<QString, int>> sorted;
        for (auto it = payloadFrequency.begin(); it != payloadFrequency.end(); ++it)
        {
            if (it.value() > 1)
            {
                sorted.append(qMakePair(it.key(), it.value()));
            }
        }
        std::sort(sorted.begin(), sorted.end(), [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
            return a.second > b.second;
        });

        QString response = QString("Trovati %1 messaggi totali. Pattern ripetuti:\n").arg(candidates.size());
        const int maxPatterns = qMin(10, sorted.size());
        for (int i = 0; i < maxPatterns; ++i)
        {
            response += QString("<br>%1x: %2").arg(sorted[i].second).arg(sorted[i].first.left(80));
        }
        result.responseHtml = response;

        for (const LogEntry &entry : candidates)
        {
            addMatch(entry);
        }
        result.indices = matchedIndices;
        result.snippets = matchedSnippets;
        return result;
    }

    const int maxResults = 200;
    for (const LogEntry &entry : candidates)
    {
        addMatch(entry);
        if (matchedIndices.size() >= maxResults)
        {
            break;
        }
    }

    result.indices = matchedIndices;
    result.snippets = matchedSnippets;

    QString response = QString("Trovati %1 messaggi corrispondenti.").arg(candidates.size());
    if (!levelTokens.isEmpty())
    {
        QStringList levelList;
        for (const QString &lvl : levelTokens)
        {
            levelList.append(lvl);
        }
        response += QString(" Livelli: %1.").arg(levelList.join(", "));
    }
    if (!categoryKeywords.isEmpty())
    {
        response += QString(" Categorie: %1.").arg(categoryKeywords.join(", "));
    }
    else if (!keywords.isEmpty())
    {
        response += QString(" Parole chiave: %1.").arg(keywords.join(", "));
    }

    QStringList indexPreview;
    const int previewCount = qMin(20, matchedIndices.size());
    if (wantsTimeline && !matchedIndices.isEmpty())
    {
        QList<int> sortedIndices = matchedIndices;
        std::sort(sortedIndices.begin(), sortedIndices.end());
        for (int i = 0; i < qMin(20, sortedIndices.size()); ++i)
        {
            indexPreview.append(QString::number(sortedIndices[i]));
        }
        response += "<br><b>Timeline (ordinati per indice):</b>";
    }
    else
    {
        for (int i = 0; i < previewCount; ++i)
        {
            indexPreview.append(QString::number(matchedIndices[i]));
        }
    }

    if (!indexPreview.isEmpty())
    {
        response += QString("<br>Indici rilevanti: %1").arg(indexPreview.join(", "));
        if (matchedIndices.size() > previewCount)
        {
            response += QString(" (+%1 altri)").arg(matchedIndices.size() - previewCount);
        }
        response += ".";
    }

    if (wantsContext)
    {
        QStringList contextLines;
        const int contextMatches = qMin(5, matchedIndices.size());
        for (int i = 0; i < contextMatches; ++i)
        {
            const int idx = matchedIndices[i];
            for (int delta = -2; delta <= 2; ++delta)
            {
                const int neighbor = idx + delta;
                if (entryByIndex.contains(neighbor))
                {
                    contextLines.append(formatEntryLine(entryByIndex.value(neighbor)).toHtmlEscaped());
                }
            }
        }
        if (!contextLines.isEmpty())
        {
            response += QString("<pre>%1</pre>").arg(contextLines.join("\n"));
        }
    }

    result.responseHtml = response;
    return result;
}

QString DltChatAnalyzer::buildHelpHtml() const
{
    QString html = "<b>Comandi disponibili:</b><br><br>";
    html += "<table border='0'>";
    html += "<tr><td><b>summary / riassumi</b></td><td>Statistiche generali dei log</td></tr>";
    html += "<tr><td><b>keywords / categorie</b></td><td>Elenco categorie di ricerca</td></tr>";
    html += "<tr><td><b>error / errore</b></td><td>Cerca tutti gli errori</td></tr>";
    html += "<tr><td><b>warn / avviso</b></td><td>Cerca tutti i warning</td></tr>";
    html += "<tr><td><b>can / ethernet / tcp</b></td><td>Cerca comunicazione di rete</td></tr>";
    html += "<tr><td><b>timeout / delay</b></td><td>Cerca problemi di timing</td></tr>";
    html += "<tr><td><b>memory / memoria</b></td><td>Cerca problemi di memoria</td></tr>";
    html += "<tr><td><b>ecu / sensor</b></td><td>Cerca componenti specifici</td></tr>";
    html += "<tr><td><b>pattern / ripeti</b></td><td>Cerca messaggi ripetuti</td></tr>";
    html += "<tr><td><b>index N</b></td><td>Vai al messaggio N</td></tr>";
    html += "<tr><td><b>context before/after</b></td><td>Mostra contesto</td></tr>";
    html += "</table>";
    html += "<br><b>Categorie keyword:</b><br>";
    for (const auto &cat : CATEGORIES)
    {
        html += QString("- %1<br>").arg(cat.name);
    }
    return html;
}

QString DltChatAnalyzer::buildCategoriesHtml() const
{
    QString html = "<b>Categorie di ricerca disponibili:</b><br><br>";
    for (const auto &cat : CATEGORIES)
    {
        html += QString("<b>%1</b>: %2<br>").arg(cat.name, cat.keywords.join(", "));
    }
    html += "<br><i>Usale direttamente nella chat per cercare!</i>";
    return html;
}

QString DltChatAnalyzer::buildSummaryHtml(const QVector<LogEntry> &entries) const
{
    QHash<QString, int> levelCounts;
    QHash<QString, int> contextCounts;
    QHash<QString, int> payloadCounts;

    for (const LogEntry &entry : entries)
    {
        levelCounts[entry.level] += 1;
        contextCounts[entry.apid + "/" + entry.ctid] += 1;
        payloadCounts[entry.payload] += 1;
    }

    auto topKeys = [](const QHash<QString, int> &counts, int limit) {
        QVector<QPair<QString, int>> pairs;
        pairs.reserve(counts.size());
        for (auto it = counts.begin(); it != counts.end(); ++it)
        {
            pairs.append(qMakePair(it.key(), it.value()));
        }
        std::sort(pairs.begin(), pairs.end(), [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
            return a.second > b.second;
        });
        QStringList list;
        for (int i = 0; i < pairs.size() && i < limit; ++i)
        {
            list.append(QString("%1 (%2)").arg(pairs[i].first).arg(pairs[i].second));
        }
        return list;
    };

    QString response = QString("Totale messaggi: %1<br>").arg(entries.size());
    response += QString("Errori: %1, Warning: %2, Info: %3, Debug: %4, Verbose: %5<br>")
        .arg(levelCounts.value("error"))
        .arg(levelCounts.value("warn"))
        .arg(levelCounts.value("info"))
        .arg(levelCounts.value("debug"))
        .arg(levelCounts.value("verbose"));

    QStringList topContexts = topKeys(contextCounts, 3);
    if (!topContexts.isEmpty())
    {
        response += QString("Contesti piu attivi: %1<br>").arg(topContexts.join(", "));
    }

    QStringList topPayloads = topKeys(payloadCounts, 3);
    if (!topPayloads.isEmpty())
    {
        response += QString("Messaggi ripetitivi: %1").arg(topPayloads.join("; "));
    }

    return response;
}

QString DltChatAnalyzer::simplifyPayload(const QString &payload)
{
    QString text = payload;
    text.replace(QChar::Null, QLatin1Char(' '));
    text = text.simplified();

    static const QRegularExpression passwordRegex("(PASSWORD\\s*[:=]\\s*)(\\S+)", QRegularExpression::CaseInsensitiveOption);
    text.replace(passwordRegex, "\\1***");

    if (text.size() > 500)
    {
        text = text.left(500) + "...";
    }
    return text;
}

QString DltChatAnalyzer::formatEntryLine(const LogEntry &entry)
{
    return QString("[%1] %2 %3 %4/%5 - %6")
        .arg(entry.index)
        .arg(entry.time)
        .arg(entry.level.toUpper())
        .arg(entry.apid)
        .arg(entry.ctid)
        .arg(entry.payload);
}
