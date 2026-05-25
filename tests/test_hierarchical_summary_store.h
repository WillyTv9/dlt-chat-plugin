#ifndef TEST_HIERARCHICAL_SUMMARY_STORE_H
#define TEST_HIERARCHICAL_SUMMARY_STORE_H

#include <QObject>

class TestHierarchicalSummaryStore : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testSetAndGetStatistics();
    void testCompactDigestEmpty();
    void testCompactDigestPopulated();
    void testCompactDigestTruncation();
    void testByTimeWindow();
};

#endif
