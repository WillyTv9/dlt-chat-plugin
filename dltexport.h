#ifndef DLTEXPORT_H
#define DLTEXPORT_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>

#include "dltanalyzerinterface.h"

class DltExport
{
public:
    static bool exportToCsv(const QString &filePath,
                            const QList<int> &indices,
                            const QStringList &snippets,
                            const QString &query,
                            const QVector<DltAnalyzerInterface::LogEntry> &entries);

    static bool exportAllEntries(const QString &filePath,
                                 const QVector<DltAnalyzerInterface::LogEntry> &entries);

    static QString generateCsvRow(const QStringList &fields);
    static QStringList sanitizeFields(const QStringList &fields);
};

#endif
