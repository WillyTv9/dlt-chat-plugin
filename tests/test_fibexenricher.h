#ifndef TEST_FIBEXENRICHER_H
#define TEST_FIBEXENRICHER_H

#include <QObject>

class TestFibexEnricher : public QObject
{
    Q_OBJECT

private slots:
    void testNotLoadedInitially();
    void testEnrichNoData();
    void testClear();
    void testEnrichEntry();
    void testLoadInvalidFile();
};

#endif
