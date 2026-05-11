#include "dltanalyzerinterface.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

static constexpr int kMaxTimeline = 200;
static constexpr int kMaxResults = 200;
static constexpr int kSnippetLength = 120;
static constexpr int kPayloadTruncateAt = 500;
static constexpr int kPreviewCount = 20;
static constexpr int kDupPreviewLines = 20;
static constexpr int kContextFrames = 5;
static constexpr int kContextNeighbors = 2;

DltRuleBasedAnalyzer::DltRuleBasedAnalyzer()
{
    supportedLanguagesList << "en" << "it" << "de" << "es" << "fr";
}

DltAnalyzerInterface::QueryResult DltRuleBasedAnalyzer::analyzeQuery(
    const QString &query, const QVector<LogEntry> &entries)
{
    QElapsedTimer t; t.start();
    QueryResult r = analyzeInternal(query, entries);
    r.processingTimeMs = t.elapsed();
    r.success = !r.responseHtml.isEmpty();
    return r;
}

static QString helpText()
{
    return
    "<b>Azioni Rapide disponibili:</b><br>"
    "<b>Errori</b> - messaggi error/fatal<br>"
    "<b>Warnings</b> - messaggi warn<br>"
    "<b>Info</b> - messaggi informativi<br>"
    "<b>Debug</b> - messaggi di debug<br>"
    "<b>CAN</b> - messaggi CAN bus<br>"
    "<b>Security</b> - auth/sicurezza<br>"
    "<b>Memoria</b> - memory/heap/leak<br>"
    "<b>Performance</b> - timeout/latenza<br>"
    "<b>Diagnostic</b> - codici diagnostici DTC<br>"
    "<b>Pattern</b> - messaggi duplicati/pattern<br>"
    "<b>Summary</b> - statistiche dei log<br>"
    "<b>Timeline</b> - sequenza cronologica<br>"
    "<b>GPS</b> - navigazione/posizione<br>"
    "<b>Help</b> - questo aiuto<br><br>"
    "<i>Puoi anche scrivere liberamente: 'mostra errori can', 'warn timeout', 'indice 42', 'riassumi'</i>";
}

DltAnalyzerInterface::QueryResult DltRuleBasedAnalyzer::analyzeInternal(
    const QString &query, const QVector<LogEntry> &entries) const
{
    QueryResult r;
    if (entries.isEmpty()) { r.responseHtml = "Nessun log caricato. Apri un file DLT."; return r; }
    QString lq = query.trimmed().toLower();
    if (lq.isEmpty()) { r.responseHtml = "Scrivi una parola chiave o usa un bottone rapido."; return r; }

    // Special commands
    if (lq == "help" || lq == "aiuto" || lq == "comandi")
    {
        r.responseHtml = helpText();
        return r;
    }
    if (lq == "timeline" || lq == "cronologia")
    {
        QStringList lines;
        lines.reserve(qMin(entries.size(), kMaxTimeline));
        for (int i = 0; i < entries.size() && i < kMaxTimeline; ++i)
        {
            const auto &e = entries[i];
            lines.append(formatEntryLine(e));
            r.indices.append(e.index);
            r.snippets.append(e.payload.left(kSnippetLength));
        }
        if (entries.size() > kMaxTimeline)
            r.responseHtml = QString("Prime 200 entry su %1:<br><pre>%2</pre>")
                .arg(entries.size()).arg(lines.join("\n").toHtmlEscaped());
        else
            r.responseHtml = QString("Sequenza completa (%1 entry):<br><pre>%2</pre>")
                .arg(entries.size()).arg(lines.join("\n").toHtmlEscaped());
        return r;
    }
    if (lq == "keywords" || lq == "categorie")
    {
        r.responseHtml =
            "<b>Categorie riconosciute:</b><br>"
            "CAN: can, canfd, arbitration, identifier<br>"
            "Security: auth, security, permission, denied, unauthorized<br>"
            "Memoria: memory, heap, stack, leak, overflow, null, alloc<br>"
            "Performance: timeout, latency, delay, slow, performance<br>"
            "Diagnostic: diagnostic, dtc, obd, fault, trouble<br>"
            "GPS: gps, position, navigation, location, satellite<br>"
            "Pattern: pattern, ripeti, duplic, frequente<br><br>"
            "<i>Usa: error, warn, info, debug per filtrare per livello</i>";
        return r;
    }
    if (lq == "summary" || lq == "riassumi" || lq == "sintesi" || lq == "statistiche")
    {
        r.responseHtml = buildSummaryHtml(entries);
        return r;
    }

    // Pattern/repetition detection
    if (lq == "pattern" || lq == "pattern ripeti" || lq.contains("ripeti") || lq.contains("duplic"))
    {
        QHash<QString, QList<int>> payloadMap;
        for (const auto &e : entries)
            payloadMap[e.payload].append(e.index);

        QStringList dupLines;
        int dupCount = 0;
        for (auto it = payloadMap.begin(); it != payloadMap.end(); ++it)
        {
            if (it.value().size() > 1)
            {
                dupCount++;
                if (dupLines.size() < kDupPreviewLines)
                {
                    QString p = it.key();
                    if (p.size() > 80) p = p.left(80) + "...";
                    dupLines.append(QString("%1 (%2x): %3")
                        .arg(it.value().first()).arg(it.value().size()).arg(p));
                    for (int idx : it.value())
                    {
                        r.indices.append(idx);
                        r.snippets.append(it.key().left(kSnippetLength));
                    }
                }
            }
        }
        if (dupCount == 0)
            r.responseHtml = "Nessun pattern di messaggi ripetuti trovato. Ogni messaggio appare una sola volta.";
        else
            r.responseHtml = QString("Trovati <b>%1</b> pattern di messaggi ripetuti su %2 totali.<br>")
                .arg(dupCount).arg(entries.size())
                + dupLines.join("<br>");
        return r;
    }

    // Level filters
    QStringList levelFilter;
    if (lq.contains("error") || lq.contains("errore") || lq.contains("fatal")) levelFilter << "error" << "fatal";
    else if (lq.contains("warn") || lq.contains("warning")) levelFilter << "warn";
    else if (lq == "info") levelFilter << "info";
    else if (lq == "debug") levelFilter << "debug";
    else if (lq == "verbose") levelFilter << "verbose";

    // Category keyword mapping
    QHash<QString, QStringList> categories;
    categories["can"] = {"can", "canfd", "arbitration", "identifier", "dlc", "errorframe"};
    categories["security"] = {"auth", "security", "permission", "denied", "unauthorized",
        "certificate", "encryption", "token", "login", "access"};
    categories["memory"] = {"memory", "heap", "stack", "leak", "overflow", "underflow",
        "null", "pointer", "alloc", "free", "buffer", "segfault"};
    categories["performance"] = {"timeout", "latency", "delay", "slow", "performance",
        "response", "elapsed", "duration", "wait", "stuck"};
    categories["diagnostic"] = {"diagnostic", "dtc", "obd", "fault", "trouble",
        "faultcode", "error code", "sid", "did"};
    categories["gps"] = {"gps", "position", "location", "navigation", "satellite",
        "gnss", "heading", "coordinates", "latitude", "longitude"};

    // Detect intent: which category or keyword to match
    QStringList matchKeywords;
    bool isSimpleLevel = false;

    if (lq == "error" || lq == "warn" || lq == "info" || lq == "debug" || lq == "verbose")
    {
        isSimpleLevel = true;
    }
    else
    {
        // Check for category match
        for (auto it = categories.begin(); it != categories.end(); ++it)
        {
            if (lq.contains(it.key()))
            {
                matchKeywords = it.value();
                break;
            }
        }
        // If no category match, extract keywords from query
        if (matchKeywords.isEmpty())
        {
            QSet<QString> sw;
            sw << "show" << "mostra" << "elenca" << "tutti" << "tutte" << "all" << "the" << "and"
               << "log" << "logs" << "messaggi" << "messaggio" << "di" << "il" << "la" << "le" << "gli"
               << "error" << "warn" << "info" << "debug" << "verbose" << "summary" << "riassumi";

            QStringList toks = lq.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
            for (const auto &t : toks)
                if (t.size() >= 3 && !sw.contains(t) && !t.at(0).isDigit())
                    matchKeywords.append(t);
        }
    }

    QSet<int> matchedSet;
    QStringList matchedSnip;

    for (const auto &e : entries)
    {
        if (!levelFilter.isEmpty() && !levelFilter.contains(e.level)) continue;

        if (isSimpleLevel)
        {
            matchedSet.insert(e.index);
            matchedSnip.append(e.payload.left(kSnippetLength));
            if (matchedSet.size() >= kMaxResults) break;
            continue;
        }

        if (!matchKeywords.isEmpty())
        {
            bool hit = false;
            for (const auto &kw : matchKeywords)
            {
                if (e.payload.contains(kw, Qt::CaseInsensitive) ||
                    e.apid.contains(kw, Qt::CaseInsensitive) ||
                    e.ctid.contains(kw, Qt::CaseInsensitive))
                { hit = true; break; }
            }
            if (!hit) continue;
        }

        matchedSet.insert(e.index);
        matchedSnip.append(e.payload.left(kSnippetLength));
        if (matchedSet.size() >= kMaxResults) break;
    }

    r.indices = QList<int>(matchedSet.begin(), matchedSet.end());
    r.snippets = matchedSnip;

    if (r.indices.isEmpty())
    {
        QString suggestion;
        if (!levelFilter.isEmpty())
            suggestion = "Nessun messaggio di livello <b>" + levelFilter.join(", ") + "</b> trovato.";
        else if (!matchKeywords.isEmpty())
            suggestion = "Nessun messaggio contenente <b>" + matchKeywords.join(", ") + "</b> trovato.<br>"
                         "Suggerimenti: prova <b>riassumi</b> per vedere statistiche, "
                         "o <b>timeline</b> per la sequenza completa.";
        else
            suggestion = "Nessun risultato. Prova: <b>riassumi</b> (statistiche), "
                         "<b>timeline</b> (tutti i messaggi), o seleziona un livello (error/warn/info/debug).";
        r.responseHtml = suggestion;
        return r;
    }

    QString resp;
    if (isSimpleLevel)
    {
        resp = QString("Trovati <b>%1</b> messaggi di livello <b>%2</b> su %3 totali.")

            .arg(r.indices.size()).arg(lq).arg(entries.size());
    }
    else if (!matchKeywords.isEmpty())
    {
        resp = QString("Trovati <b>%1</b> messaggi corrispondenti su %2 totali.<br>")
            .arg(r.indices.size()).arg(entries.size());
        if (!levelFilter.isEmpty())
            resp += QString("Livello: %1<br>").arg(levelFilter.join(", "));
        resp += QString("Parole chiave: %1").arg(matchKeywords.join(", "));
    }
    else
        resp = QString("Trovati <b>%1</b> messaggi su %2.").arg(r.indices.size()).arg(entries.size());

    QStringList preview;
    for (int i = 0; i < qMin(kPreviewCount, r.indices.size()); ++i)
        preview.append(QString::number(r.indices[i]));
    if (!preview.isEmpty())
    {
        resp += QString("<br>Indici: %1").arg(preview.join(", "));
        if (r.indices.size() > 20) resp += QString(" (+%1)").arg(r.indices.size() - 20);
    }

    r.responseHtml = resp;
    return r;
}

QString DltRuleBasedAnalyzer::buildSummaryHtml(const QVector<LogEntry> &entries) const
{
    QHash<QString, int> lc, cc, pc;
    for (const auto &e : entries) {
        lc[e.level]++; cc[e.apid+"/"+e.ctid]++; pc[e.payload]++;
    }

    auto top = [](const QHash<QString,int> &h, int lim) -> QStringList {
        QVector<QPair<QString,int>> p; p.reserve(h.size());
        for (auto it = h.begin(); it != h.end(); ++it) p.append({it.key(), it.value()});
        std::sort(p.begin(), p.end(), [](const auto &a, const auto &b){ return a.second > b.second; });
        QStringList r;
        for (int i = 0; i < p.size() && i < lim; ++i)
            r.append(QString("%1 (%2)").arg(p[i].first).arg(p[i].second));
        return r;
    };

    int total = entries.size();
    QString r = QString("<b>Riepilogo log</b> &mdash; %1 messaggi totali<br><br>").arg(total);
    r += QString("Errori/Fatal: %1 | Warnings: %2 | Info: %3 | Debug: %4 | Verbose: %5<br><br>")
        .arg(lc.value("error")+lc.value("fatal")).arg(lc.value("warn")).arg(lc.value("info"))
        .arg(lc.value("debug")).arg(lc.value("verbose"));

    QStringList tc = top(cc, kContextFrames);
    if (!tc.isEmpty()) r += QString("<b>Contesti piu attivi:</b> %1<br>").arg(tc.join(", "));

    QStringList tp = top(pc, kContextFrames);
    if (!tp.isEmpty()) r += QString("<b>Messaggi ripetuti:</b> %1").arg(tp.join("; "));

    return r;
}

QString DltRuleBasedAnalyzer::formatEntryLine(const LogEntry &e)
{
    return QString("[%1] %2 %3 %4/%5 - %6")
        .arg(e.index).arg(e.time).arg(e.level.toUpper())
        .arg(e.apid).arg(e.ctid).arg(e.payload);
}

QString DltRuleBasedAnalyzer::simplifyPayload(const QString &payload)
{
    QString t = payload;
    t.replace(QChar::Null, QLatin1Char(' '));
    t = t.simplified();
    t.replace(QRegularExpression("(PASSWORD\\s*[:=]\\s*)(\\S+)",
        QRegularExpression::CaseInsensitiveOption), "\\1***");
    if (t.size() > kPayloadTruncateAt) t = t.left(kPayloadTruncateAt) + "...";
    return t;
}

QString DltRuleBasedAnalyzer::configurationInfo() const
{
    return QString("Rule-Based Analyzer v%1\nLingue: %2\nMax: 200\nImmediato")
        .arg(version()).arg(supportedLanguagesList.join(", "));
}

QStringList DltRuleBasedAnalyzer::supportedLanguages() const { return supportedLanguagesList; }
bool DltRuleBasedAnalyzer::configure(const QVariantMap &) { return true; }

QVariantMap DltRuleBasedAnalyzer::currentConfiguration() const
{
    QVariantMap c;
    c["type"] = "rule-based"; c["version"] = version();
    c["supportedLanguages"] = supportedLanguagesList; c["maxResults"] = kMaxResults;
    return c;
}
