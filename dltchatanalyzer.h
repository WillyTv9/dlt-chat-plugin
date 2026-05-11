#ifndef DLTCHATANALYZER_H
#define DLTCHATANALYZER_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

class DltChatAnalyzer
{
public:
    struct LogEntry
    {
        int index = -1;
        QString time;
        QString timestamp;
        QString ecu;
        QString apid;
        QString ctid;
        QString level;
        QString payload;
    };

    struct QueryResult
    {
        QString responseHtml;
        QList<int> indices;
        QStringList snippets;
    };

    QueryResult analyzeQuery(const QString &query, const QVector<LogEntry> &entries) const;

    static QString simplifyPayload(const QString &payload);
    static QString formatEntryLine(const LogEntry &entry);

    QStringList getAllKeywords() const;
    QStringList getAllCategories() const;
    QStringList getKeywordsForCategory(const QString &category) const;

private:
    QString buildSummaryHtml(const QVector<LogEntry> &entries) const;
    QString buildHelpHtml() const;
    QString buildCategoriesHtml() const;
};

#endif // DLTCHATANALYZER_H
