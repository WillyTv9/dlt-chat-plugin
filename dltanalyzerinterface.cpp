#include "dltanalyzerinterface.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <utility>

static constexpr int kMaxTimeline = 200;
static constexpr int kMaxSnippets = 5000;
static constexpr int kSnippetLength = 120;
static constexpr int kPayloadTruncateAt = 500;
static constexpr int kPreviewCount = 20;
static constexpr int kDupPreviewLines = 20;
static constexpr int kContextFrames = 10;
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
    "<i>Puoi anche combinare: 'mostra errori can', 'warn timeout', 'info carplay'</i>";
}

static QStringList extractLevels(const QString &lq)
{
    QStringList levels;
    if (lq.contains("error") || lq.contains("errore") || lq.contains("fatal") || lq.contains("fatale"))
        levels << "error" << "fatal";
    if (lq.contains("warn") || lq.contains("warning") || lq.contains("avviso"))
        levels << "warn";
    if (lq.contains("info"))
        levels << "info";
    if (lq.contains("debug"))
        levels << "debug";
    if (lq.contains("verbose"))
        levels << "verbose";
    return levels;
}

static QStringList extractDomains(const QString &lq)
{
    QStringList domains;
    if (lq.contains("carplay")) domains << "carplay";
    if (lq.contains("androidauto") || lq.contains("android auto") || lq.contains("aa")) domains << "androidauto";
    return domains;
}

struct CategoryRule {
    QString name;
    QStringList keywords;
};

static const QVector<CategoryRule> &allCategories()
{
    static QVector<CategoryRule> cats;
    if (cats.isEmpty()) {
        cats = {
            {"can",         {"can", "canfd", "arbitration", "identifier", "dlc", "errorframe"}},
            {"security",    {"auth", "security", "permission", "denied", "unauthorized",
                             "certificate", "encryption", "token", "login", "access"}},
            {"memory",      {"memory", "heap", "stack", "leak", "overflow", "underflow",
                             "null", "pointer", "alloc", "free", "buffer", "segfault"}},
            {"performance", {"timeout", "latency", "delay", "slow", "performance",
                             "response", "elapsed", "duration", "wait", "stuck"}},
            {"diagnostic",  {"diagnostic", "dtc", "obd", "fault", "trouble",
                             "faultcode", "error code", "sid", "did"}},
            {"gps",         {"gps", "position", "location", "navigation", "satellite",
                             "gnss", "heading", "coordinates", "latitude", "longitude"}},
        };
    }
    return cats;
}

static QStringList extractCategoryKeywords(const QString &lq)
{
    QStringList keywords;
    for (const auto &cat : allCategories()) {
        if (lq.contains(cat.name)) {
            keywords.append(cat.keywords);
            break;
        }
    }
    if (keywords.isEmpty()) {
        QSet<QString> sw;
        sw << "show" << "mostra" << "elenca" << "tutti" << "tutte" << "all" << "the" << "and"
           << "log" << "logs" << "messaggi" << "messaggio" << "di" << "il" << "la" << "le" << "gli"
           << "dei" << "delle" << "degli" << "una" << "uno" << "un" << "per" << "con" << "che"
           << "error" << "warn" << "info" << "debug" << "verbose" << "summary" << "riassumi"
           << "sintesi" << "statistiche" << "help" << "aiuto" << "comandi" << "timeline" << "cronologia"
           << "carplay" << "androidauto" << "pattern" << "categorie" << "keywords";

        QStringList toks = lq.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
        for (const auto &t : toks)
            if (t.size() >= 3 && !sw.contains(t) && !t.at(0).isDigit())
                keywords.append(t);
    }
    return keywords;
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
        QString html = "<b>Categorie riconosciute:</b><br>"
            "CAN: can, canfd, arbitration, identifier<br>"
            "Security: auth, security, permission, denied, unauthorized<br>"
            "Memoria: memory, heap, stack, leak, overflow, null, alloc<br>"
            "Performance: timeout, latency, delay, slow, performance<br>"
            "Diagnostic: diagnostic, dtc, obd, fault, trouble<br>"
            "GPS: gps, position, navigation, location, satellite<br><br>"
            "<b>Domini riconosciuti:</b><br>"
            "CarPlay: carplay, apple carplay<br>"
            "Android Auto: androidauto, android auto<br><br>"
            "<i>Combina: 'error can', 'warn carplay', 'info security timeout'</i>";
        r.responseHtml = html;
        return r;
    }
    if (lq == "summary" || lq == "riassumi" || lq == "sintesi" || lq == "statistiche")
    {
        r.responseHtml = buildSummaryHtml(entries);
        return r;
    }

    // Pattern/repetition detection (process ALL entries, no limit)
    if (lq == "pattern" || lq == "pattern ripeti" || lq.contains("ripeti") || lq.contains("duplic"))
    {
        QHash<QString, QList<int>> payloadMap;
        payloadMap.reserve(entries.size() / 2);
        for (const auto &e : entries)
            payloadMap[e.payload].append(e.index);

        QStringList dupLines;
        int dupCount = 0;
        int totalIndices = 0;
        r.indices.reserve(entries.size() / 2);
        for (auto it = payloadMap.begin(); it != payloadMap.end(); ++it)
        {
            if (it.value().size() > 1)
            {
                dupCount++;
                for (int idx : it.value())
                {
                    r.indices.append(idx);
                    totalIndices++;
                    if (r.snippets.size() < kMaxSnippets)
                        r.snippets.append(it.key().left(kSnippetLength));
                }
                if (dupLines.size() < kDupPreviewLines)
                {
                    QString p = it.key();
                    if (p.size() > 80) p = p.left(80) + "...";
                    dupLines.append(QString("%1 (%2x): %3")
                        .arg(it.value().first()).arg(it.value().size()).arg(p));
                }
            }
        }
        if (dupCount == 0)
            r.responseHtml = "Nessun pattern di messaggi ripetuti trovato. Ogni messaggio appare una sola volta.";
        else
            r.responseHtml = QString("Trovati <b>%1</b> pattern di messaggi ripetuti su %2 totali, "
                                     "coprendo <b>%3</b> messaggi.<br>")
                .arg(dupCount).arg(entries.size()).arg(totalIndices)
                + dupLines.join("<br>");
        return r;
    }

    // --- Combined detection: level + category + domain ---
    QStringList levelFilter = extractLevels(lq);
    QStringList domainFilter = extractDomains(lq);
    QStringList matchKeywords = extractCategoryKeywords(lq);

    bool isSimpleLevel = levelFilter.size() == 1 && matchKeywords.isEmpty() && domainFilter.isEmpty();

    QSet<int> matchedSet;
    QStringList matchedSnip;

    for (const auto &e : entries)
    {
        if (!levelFilter.isEmpty() && !levelFilter.contains(e.level)) continue;
        if (!domainFilter.isEmpty() && !domainFilter.contains(e.domain)) continue;

        if (isSimpleLevel && levelFilter.size() == 1)
        {
            matchedSet.insert(e.index);
            if (matchedSnip.size() < kMaxSnippets)
                matchedSnip.append(e.payload.left(kSnippetLength));
            continue;
        }

        if (!matchKeywords.isEmpty())
        {
            bool hit = false;
            for (const auto &kw : matchKeywords)
            {
                if (e.payload.contains(kw, Qt::CaseInsensitive) ||
                    e.apid.contains(kw, Qt::CaseInsensitive) ||
                    e.ctid.contains(kw, Qt::CaseInsensitive) ||
                    e.domain.contains(kw, Qt::CaseInsensitive))
                { hit = true; break; }
            }
            if (!hit) continue;
        }

        matchedSet.insert(e.index);
        if (matchedSnip.size() < kMaxSnippets)
            matchedSnip.append(e.payload.left(kSnippetLength));
    }

    r.indices = QList<int>(matchedSet.begin(), matchedSet.end());
    r.snippets = matchedSnip;

    if (r.indices.isEmpty())
    {
        QString suggestion;
        if (!levelFilter.isEmpty())
            suggestion = "Nessun messaggio di livello <b>" + levelFilter.join(", ") + "</b> trovato.";
        else if (!domainFilter.isEmpty())
            suggestion = "Nessun messaggio per il dominio <b>" + domainFilter.join(", ") + "</b> trovato.";
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
    if (isSimpleLevel && levelFilter.size() == 1)
    {
        resp = QString("Trovati <b>%1</b> messaggi di livello <b>%2</b> su %3 totali.")
            .arg(r.indices.size()).arg(levelFilter.first()).arg(entries.size());
    }
    else
    {
        resp = QString("Trovati <b>%1</b> messaggi corrispondenti su %2 totali.<br>")
            .arg(r.indices.size()).arg(entries.size());
        if (!levelFilter.isEmpty())
            resp += QString("Livello: %1<br>").arg(levelFilter.join(", "));
        if (!domainFilter.isEmpty())
            resp += QString("Dominio: %1<br>").arg(domainFilter.join(", "));
        if (!matchKeywords.isEmpty())
            resp += QString("Parole chiave: %1").arg(matchKeywords.join(", "));
    }

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
    QHash<QString, int> lc, cc, pc, dc;
    QString firstTime, lastTime;
    for (const auto &e : entries) {
        lc[e.level]++;
        cc[e.apid+"/"+e.ctid]++;
        pc[e.payload]++;
        dc[e.domain]++;
        if (firstTime.isEmpty() || e.time < firstTime) firstTime = e.time;
        if (lastTime.isEmpty() || e.time > lastTime) lastTime = e.time;
    }

    auto top = [](const QHash<QString,int> &h, int lim) -> QStringList {
        if (h.isEmpty()) return {};
        QVector<QPair<QString,int>> p; p.reserve(h.size());
        for (auto it = h.begin(); it != h.end(); ++it) p.append({it.key(), it.value()});
        std::sort(p.begin(), p.end(), [](const auto &a, const auto &b){ return a.second > b.second; });
        QStringList r;
        for (int i = 0; i < p.size() && i < lim; ++i)
            r.append(QString("%1 (%2)").arg(p[i].first).arg(p[i].second));
        return r;
    };

    int total = entries.size();
    int errors = lc.value("error") + lc.value("fatal");
    int warns = lc.value("warn");
    QString r = QString("<b>Riepilogo log</b> &mdash; %1 messaggi totali<br><br>").arg(total);

    if (!firstTime.isEmpty() && !lastTime.isEmpty())
        r += QString("<b>Intervallo:</b> %1 &ndash; %2<br><br>").arg(firstTime, lastTime);

    r += QString("<b>Livelli:</b> Errori/Fatal: %1 | Warnings: %2 | Info: %3 | Debug: %4 | Verbose: %5<br><br>")
        .arg(errors).arg(warns).arg(lc.value("info")).arg(lc.value("debug")).arg(lc.value("verbose"));

    if (!dc.isEmpty()) {
        QStringList ds;
        for (auto it = dc.begin(); it != dc.end(); ++it)
            if (!it.key().isEmpty()) ds.append(QString("%1: %2").arg(it.key()).arg(it.value()));
        if (!ds.isEmpty())
            r += QString("<b>Domini:</b> %1<br><br>").arg(ds.join(" | "));
    }

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
    return QString("Rule-Based Analyzer v%1\nLingue: %2\nAnalisi completa (no limit)\nImmediato")
        .arg(version()).arg(supportedLanguagesList.join(", "));
}

QStringList DltRuleBasedAnalyzer::supportedLanguages() const { return supportedLanguagesList; }
bool DltRuleBasedAnalyzer::configure(const QVariantMap &) { return true; }

QVariantMap DltRuleBasedAnalyzer::currentConfiguration() const
{
    QVariantMap c;
    c["type"] = "rule-based"; c["version"] = version();
    c["supportedLanguages"] = supportedLanguagesList;
    return c;
}
