#include "dltanalyzerinterface.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>

DltRuleBasedAnalyzer::DltRuleBasedAnalyzer()
{
    supportedLanguagesList << "en" << "it" << "de" << "es" << "fr";
}

DltAnalyzerInterface::QueryResult DltRuleBasedAnalyzer::analyzeQuery(
    const QString &query,
    const QVector<LogEntry> &entries)
{
    QElapsedTimer timer;
    timer.start();

    QueryResult result = analyzeInternal(query, entries);

    result.processingTimeMs = timer.elapsed();
    result.success = !result.responseHtml.isEmpty();

    return result;
}

DltAnalyzerInterface::QueryResult DltRuleBasedAnalyzer::analyzeInternal(
    const QString &query,
    const QVector<LogEntry> &entries) const
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

    const bool isSummary = lowerQuery.contains("summary")
        || lowerQuery.contains("summarize")
        || lowerQuery.contains("riassumi")
        || lowerQuery.contains("sintesi");

    if (isSummary)
    {
        result.responseHtml = buildSummaryHtml(entries);
        return result;
    }

    QSet<QString> levelTokens;
    if (lowerQuery.contains("fatal") || lowerQuery.contains("fatale") || lowerQuery.contains("critico"))
    {
        levelTokens.insert("fatal");
        levelTokens.insert("error");
    }
    if (lowerQuery.contains("error") || lowerQuery.contains("errore"))
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
        || lowerQuery.contains("motivo");

    QSet<QString> stopwords;
    stopwords << "show" << "mostra" << "elenca" << "tutti" << "tutte" << "all"
              << "why" << "perche" << "causa" << "motivo" << "summary" << "summarize"
              << "riassumi" << "sintesi" << "log" << "logs" << "messaggi" << "messaggio"
              << "indice" << "index" << "riga" << "line" << "timestamp" << "time" << "tempo"
              << "error" << "errors" << "errore" << "errori" << "fatal" << "fatale" << "warn" << "warning"
              << "avviso" << "info" << "debug" << "verbose";

    QStringList tokens = lowerQuery.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    QStringList keywords;
    for (const QString &token : tokens)
    {
        if (token.size() < 3)
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
        if (!keywords.isEmpty())
        {
            bool hit = false;
            for (const QString &kw : keywords)
            {
                if (entry.payload.contains(kw, Qt::CaseInsensitive))
                {
                    hit = true;
                    break;
                }
            }
            if (!hit)
            {
                continue;
            }
        }
        candidates.append(entry);
    }

    if (candidates.isEmpty())
    {
        result.responseHtml = "Nessun risultato. Prova con parole chiave diverse o usa 'riassumi'.";
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
    if (!keywords.isEmpty())
    {
        response += QString(" Parole chiave: %1.").arg(keywords.join(", "));
    }

    QStringList indexPreview;
    const int previewCount = qMin(20, matchedIndices.size());
    for (int i = 0; i < previewCount; ++i)
    {
        indexPreview.append(QString::number(matchedIndices[i]));
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

QString DltRuleBasedAnalyzer::buildSummaryHtml(const QVector<LogEntry> &entries) const
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

QString DltRuleBasedAnalyzer::formatEntryLine(const LogEntry &entry) const
{
    return QString("[%1] %2 %3 %4/%5 - %6")
        .arg(entry.index)
        .arg(entry.time)
        .arg(entry.level.toUpper())
        .arg(entry.apid)
        .arg(entry.ctid)
        .arg(entry.payload);
}

QString DltRuleBasedAnalyzer::simplifyPayload(const QString &payload)
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

QString DltRuleBasedAnalyzer::configurationInfo() const
{
    return QString("Rule-Based Analyzer v%1\n"
                  "- Multi-language support: %2\n"
                  "- Max results per query: %3\n"
                  "- Real-time processing: Si")
        .arg(version())
        .arg(supportedLanguagesList.join(", "))
        .arg(200);
}

QStringList DltRuleBasedAnalyzer::supportedLanguages() const
{
    return supportedLanguagesList;
}

bool DltRuleBasedAnalyzer::configure(const QVariantMap &config)
{
    Q_UNUSED(config);
    return true;
}

QVariantMap DltRuleBasedAnalyzer::currentConfiguration() const
{
    QVariantMap config;
    config["type"] = "rule-based";
    config["version"] = version();
    config["supportedLanguages"] = supportedLanguagesList;
    config["maxResults"] = 200;
    return config;
}
