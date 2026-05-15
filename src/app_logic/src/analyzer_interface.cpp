#include "dltchat/analyzer_interface.h"
#include "dltchat/category_registry.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <utility>

namespace dltchat {

static constexpr int kMaxTimeline = 10000000;
static constexpr int kMaxSnippets = 10000000;
static constexpr int kSnippetLength = 120;
static constexpr int kPayloadTruncateAt = 500;
static constexpr int kPreviewCount = 20;
static constexpr int kDupPreviewLines = 20;
static constexpr int kContextFrames = 10;
static constexpr int kContextNeighbors = 2;

struct ErrorCategory {
    QString name;
    QStringList patterns;
};

static const QVector<ErrorCategory> &errorCategories()
{
    static QVector<ErrorCategory> cats;
    if (cats.isEmpty()) {
        cats = {
            {"Comunicazione",  {"timeout", "connection", "disconnect", "handshake",
                                "no response", "link down", "bus-off", "lost"}},
            {"Memoria",        {"memory", "heap", "stack", "leak", "overflow",
                                "null pointer", "alloc", "segfault", "out of memory"}},
            {"Sicurezza",      {"auth", "security", "permission", "denied",
                                "unauthorized", "certificate", "encryption", "token"}},
            {"Configurazione", {"configuration", "config", "invalid param",
                                "wrong", "unknown", "unexpected", "mismatch"}},
            {"Hardware",       {"hardware", "sensor", "actuator", "driver",
                                "i2c", "spi", "gpio", "adc", "dac"}},
            {"Timeout",        {"timeout", "timed out", "expired", "retry",
                                "no ack", "no response"}},
            {"Protocollo",     {"protocol", "checksum", "crc", "framing",
                                "invalid message", "malformed", "unexpected data"}},
        };
    }
    return cats;
}

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
    "<b>Verbose</b> - messaggi verbose<br>"
    "<b>CAN</b> - messaggi CAN bus<br>"
    "<b>Security</b> - auth/sicurezza<br>"
    "<b>Memoria</b> - memory/heap/leak<br>"
    "<b>Performance</b> - timeout/latenza<br>"
    "<b>Diagnostic</b> - codici diagnostici DTC<br>"
    "<b>GPS</b> - navigazione/posizione<br>"
    "<b>Pattern</b> - messaggi duplicati/pattern<br>"
    "<b>Summary</b> - statistiche dei log<br>"
    "<b>Timeline</b> - sequenza cronologica<br>"
    "<b>Categorizza</b> - classifica errori per categoria<br>"
    "<b>CarPlay</b> - eventi sessione CarPlay<br>"
    "<b>AndroidAuto</b> - eventi Android Auto<br>"
    "<b>Focus</b> - video focus perso<br>"
    "<b>Ducking</b> - audio ducking<br>"
    "<b>mDNS</b> - handshake mDNS<br>"
    "<b>Sensor</b> - dati sensori veicolo<br>"
    "<b>Auth Errors</b> - errori autenticazione<br>"
    "<b>Session</b> - eventi inizio/fine sessione<br>"
    "<b>Keywords</b> - parole chiave disponibili<br>"
    "<b>Categories</b> - categorie disponibili<br>"
    "<b>Help</b> - questo aiuto<br><br>"
    "<i>Puoi anche combinare: 'error can', 'warn timeout', 'info carplay'</i>";
}

static QStringList extractLevels(const QString &lq)
{
    QStringList levels;
    const auto resolved = CategoryRegistry::instance().resolveQuery(lq);
    if (resolved.kind == ResolvedQuery::Kind::Category && !resolved.categoryIds.isEmpty()) {
        const auto *cat = CategoryRegistry::instance().categoryById(resolved.categoryId);
        if (cat && cat->levelOnly) {
            for (const auto &f : cat->filters) {
                if (f == QLatin1String("error") || f == QLatin1String("fatal")
                    || f == QLatin1String("lerr") || f == QLatin1String("err"))
                    levels << "error" << "fatal";
                else if (f == QLatin1String("warn") || f == QLatin1String("lwarn")
                         || f == QLatin1String("warning"))
                    levels << "warn";
                else if (f == QLatin1String("info") || f == QLatin1String("linf"))
                    levels << "info";
                else if (f == QLatin1String("debug") || f == QLatin1String("ldebug")
                         || f == QLatin1String("dbg"))
                    levels << "debug";
                else if (f == QLatin1String("verbose"))
                    levels << "verbose";
            }
            levels.removeDuplicates();
            if (!levels.isEmpty())
                return levels;
        }
    }
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
    if (lq.contains("androidauto") || lq.contains("android auto")) domains << "androidauto";
    return domains;
}

static QStringList extractCategoryKeywords(const QString &lq)
{
    const auto resolved = CategoryRegistry::instance().resolveQuery(lq);
    if (resolved.kind == ResolvedQuery::Kind::Category && !resolved.categoryId.isEmpty()) {
        const auto *cat = CategoryRegistry::instance().categoryById(resolved.categoryId);
        if (cat && !cat->levelOnly)
            return cat->filters;
    }
    if (resolved.kind == ResolvedQuery::Kind::CombinedFilter) {
        QStringList keywords = resolved.extraFilters;
        for (const auto &cid : resolved.categoryIds) {
            const auto *cat = CategoryRegistry::instance().categoryById(cid);
            if (cat)
                keywords.append(cat->filters);
        }
        return keywords;
    }

    QStringList keywords;
    QSet<QString> sw;
    sw << "show" << "mostra" << "elenca" << "tutti" << "tutte" << "all" << "the" << "and"
       << "log" << "logs" << "messaggi" << "messaggio" << "di" << "il" << "la" << "le" << "gli"
       << "dei" << "delle" << "degli" << "una" << "uno" << "un" << "per" << "con" << "che"
       << "error" << "warn" << "info" << "debug" << "verbose" << "summary" << "riassumi"
       << "sintesi" << "statistiche" << "help" << "aiuto" << "comandi" << "timeline" << "cronologia"
       << "pattern" << "categorie" << "keywords";

    QStringList toks = lq.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    for (const auto &t : toks)
        if (t.size() >= 3 && !sw.contains(t) && !t.at(0).isDigit())
            keywords.append(t);
    return keywords;
}

DltAnalyzerInterface::QueryResult DltRuleBasedAnalyzer::analyzeInternal(
    const QString &query, const QVector<LogEntry> &entries) const
{
    QueryResult r;
    if (entries.isEmpty()) { r.responseHtml = "Nessun log caricato. Apri un file DLT."; return r; }
    QString lq = query.trimmed().toLower();
    if (lq.isEmpty()) { r.responseHtml = "Scrivi una parola chiave o usa un bottone rapido."; return r; }

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
    if (lq == "keywords" || lq == "categorie" || lq == "categories")
    {
        r.responseHtml = CategoryRegistry::instance().buildCategoriesHelpHtml();
        return r;
    }
    if (lq == "summary" || lq == "riassumi" || lq == "sintesi" || lq == "statistiche")
    {
        r.responseHtml = buildSummaryHtml(entries);
        return r;
    }

    if (lq == "categorizza" || lq == "categorize" || lq == "classifica")
    {
        QVector<LogEntry> errors;
        for (const auto &e : entries)
            if (e.level == "error" || e.level == "fatal")
                errors.append(e);

        if (errors.isEmpty())
        {
            r.responseHtml = "Nessun errore o fatal da categorizzare.";
            return r;
        }

        QHash<QString, QList<int>> categorized;
        QList<int> uncategorized;
        for (const auto &e : errors)
        {
            bool matched = false;
            for (const auto &cat : errorCategories())
            {
                for (const auto &pat : cat.patterns)
                {
                    if (e.payload.contains(pat, Qt::CaseInsensitive))
                    {
                        categorized[cat.name].append(e.index);
                        r.indices.append(e.index);
                        r.snippets.append(e.payload.left(kSnippetLength));
                        matched = true;
                        break;
                    }
                }
                if (matched) break;
            }
            if (!matched) {
                uncategorized.append(e.index);
                r.indices.append(e.index);
                r.snippets.append(e.payload.left(kSnippetLength));
            }
        }

        QString html = QString("<b>Categorizzazione errori</b> &mdash; %1 errori/fatal su %2 totali<br><br>")
            .arg(errors.size()).arg(entries.size());

        for (auto it = categorized.begin(); it != categorized.end(); ++it)
            html += QString("<b>%1:</b> %2 messaggi (es. indice %3)<br>")
                .arg(it.key()).arg(it.value().size()).arg(it.value().first());

        if (!uncategorized.isEmpty())
            html += QString("<br><b>Non categorizzati:</b> %1 messaggi<br>")
                .arg(uncategorized.size());

        std::sort(r.indices.begin(), r.indices.end());
        r.responseHtml = html;
        return r;
    }

    // --- pattern / duplicate detection ---
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
        std::sort(r.indices.begin(), r.indices.end());
        if (dupCount == 0)
            r.responseHtml = "Nessun pattern di messaggi ripetuti trovato. Ogni messaggio appare una sola volta.";
        else
            r.responseHtml = QString("Trovati <b>%1</b> pattern di messaggi ripetuti su %2 totali, "
                                     "coprendo <b>%3</b> messaggi.<br>")
                .arg(dupCount).arg(entries.size()).arg(totalIndices)
                + dupLines.join("<br>");
        return r;
    }

    QStringList levelFilter = extractLevels(lq);
    QStringList domainFilter = extractDomains(lq);
    QStringList matchKeywords = extractCategoryKeywords(lq);

    // If a domain is detected (e.g. "carplay"), also use it as a keyword
    // so entries containing the word in payload/apid/ctid are found even
    // if AutomotiveLogParser did not classify them under that domain.
    if (!domainFilter.isEmpty() && matchKeywords.isEmpty()) {
        for (const auto &d : domainFilter)
            matchKeywords.append(d);
    }

    bool isSimpleLevel = levelFilter.size() == 1 && matchKeywords.isEmpty() && domainFilter.isEmpty();

    QSet<int> matchedSet;
    QStringList matchedSnip;

    for (const auto &e : entries)
    {
        if (!levelFilter.isEmpty() && !levelFilter.contains(e.level)) continue;
        if (!domainFilter.isEmpty()) {
            bool domainHit = domainFilter.contains(e.domain);
            if (!domainHit) {
                for (const auto &d : domainFilter) {
                    if (e.payload.contains(d, Qt::CaseInsensitive) ||
                        e.apid.contains(d, Qt::CaseInsensitive) ||
                        e.ctid.contains(d, Qt::CaseInsensitive))
                    { domainHit = true; break; }
                }
            }
            if (!domainHit) continue;
        }

        if (isSimpleLevel && levelFilter.size() == 1)
        {
            matchedSet.insert(e.index);
            if (matchedSnip.size() < kMaxSnippets)
                matchedSnip.append(e.payload.left(kSnippetLength));
            continue;
        }

        if (!matchKeywords.isEmpty() && domainFilter.isEmpty())
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
    std::sort(r.indices.begin(), r.indices.end());
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

} // namespace dltchat

