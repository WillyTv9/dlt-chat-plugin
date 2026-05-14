#ifndef TEST_DLTEXPORT_H
#define TEST_DLTEXPORT_H

#include <QObject>
#include <QVector>
#include "dltchat/analyzer_interface.h"

class TestDltExport : public QObject
{
    Q_OBJECT

private:
    QVector<dltchat::DltAnalyzerInterface::LogEntry> makeEntries() const;
    static QString generateCsvRow(const QStringList &fields);
    static QStringList sanitizeFields(const QStringList &fields);

private slots:
    void testCsvRowGeneration();
    void testCsvRowEscaping();
    void testSanitizeFields();
    void testExportToCsv();
    void testExportAllEntries();
    void testExportEmptyIndices();
    void testExportEmptyEntries();
};

#endif
