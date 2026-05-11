#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "dltchatanalyzer.h"

static QVector<DltChatAnalyzer::LogEntry> parseLogFile(const QString &path)
{
    QVector<DltChatAnalyzer::LogEntry> entries;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return entries;
    }

    QTextStream stream(&file);
    QRegularExpression lineRegex("^(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}),(\\d{3})\\s+(\\w+)\\s+\\[(.+?)\\]\\s+(.*)$");

    while (!stream.atEnd())
    {
        const QString line = stream.readLine();
        QRegularExpressionMatch match = lineRegex.match(line);
        if (match.hasMatch())
        {
            DltChatAnalyzer::LogEntry entry;
            entry.index = entries.size();
            entry.time = QString("%1.%2").arg(match.captured(1)).arg(match.captured(2));
            entry.timestamp = entry.time;
            entry.level = match.captured(3).toLower();
            entry.apid = match.captured(4);
            entry.ctid = QString();
            entry.ecu = QString();
            entry.payload = DltChatAnalyzer::simplifyPayload(match.captured(5));
            entries.append(entry);
        }
        else if (!entries.isEmpty() && !line.trimmed().isEmpty())
        {
            DltChatAnalyzer::LogEntry &entry = entries.last();
            entry.payload = DltChatAnalyzer::simplifyPayload(entry.payload + " " + line.trimmed());
        }
    }

    return entries;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 2)
    {
        qCritical("Usage: dltchatplugin_test <logfile>");
        return 2;
    }

    const QString path = QString::fromLocal8Bit(argv[1]);
    const QVector<DltChatAnalyzer::LogEntry> entries = parseLogFile(path);
    if (entries.isEmpty())
    {
        qCritical("No entries parsed from log file.");
        return 3;
    }

    int errorLevelCount = 0;
    for (const auto &entry : entries)
    {
        if (entry.level == "error")
        {
            ++errorLevelCount;
        }
    }
    qInfo("Parsed entries=%d error_level=%d", entries.size(), errorLevelCount);

    DltChatAnalyzer analyzer;

    const DltChatAnalyzer::QueryResult errorResult = analyzer.analyzeQuery("mostra errori", entries);
    if (errorResult.indices.isEmpty())
    {
        qCritical("Expected errors not found.");
        return 4;
    }

    const DltChatAnalyzer::QueryResult summaryResult = analyzer.analyzeQuery("riassumi", entries);
    if (!summaryResult.responseHtml.contains("Totale messaggi"))
    {
        qCritical("Summary did not include total messages.");
        return 5;
    }

    const DltChatAnalyzer::QueryResult missingIndexResult = analyzer.analyzeQuery("indice 999999", entries);
    if (!missingIndexResult.responseHtml.contains("Indice non trovato"))
    {
        qCritical("Missing index error not reported.");
        return 6;
    }

    const DltChatAnalyzer::QueryResult emptyQueryResult = analyzer.analyzeQuery("   ", entries);
    if (!emptyQueryResult.responseHtml.contains("Inserisci una domanda"))
    {
        qCritical("Empty query was not rejected.");
        return 7;
    }

    const DltChatAnalyzer::QueryResult keywordResult = analyzer.analyzeQuery("timeout", entries);
    if (keywordResult.indices.isEmpty())
    {
        qCritical("Timeout keyword did not return any results.");
        return 8;
    }

    QVector<DltChatAnalyzer::LogEntry> stressEntries = entries;
    const int targetSize = 20000;
    stressEntries.reserve(targetSize);
    while (stressEntries.size() < targetSize)
    {
        for (const auto &entry : entries)
        {
            stressEntries.append(entry);
            if (stressEntries.size() >= targetSize)
            {
                break;
            }
        }
    }

    QElapsedTimer timer;
    timer.start();
    analyzer.analyzeQuery("mostra errori", stressEntries);
    qInfo("Stress test entries=%d duration_ms=%lld", stressEntries.size(), timer.elapsed());

    qInfo("All tests passed.");
    return 0;
}
