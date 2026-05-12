#ifndef TEST_DLTEXPORT_H
#define TEST_DLTEXPORT_H

#include <QObject>
#include <QVector>
#include "dltanalyzerinterface.h"

class TestDltExport : public QObject
{
    Q_OBJECT

private:
    QVector<DltAnalyzerInterface::LogEntry> makeEntries() const;

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
