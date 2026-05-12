#ifndef TEST_CONTEXTUALEXTRACTOR_H
#define TEST_CONTEXTUALEXTRACTOR_H

#include <QObject>

class TestContextualExtractor : public QObject
{
    Q_OBJECT

private slots:
    void testExtractContextWithSelectedIndices();
    void testExtractContextNoQueryReturnsEmpty();
    void testWindowExpansion();
    void testGroupsBuiltCorrectly();
    void testSortByTimestamp();
    void testMaxEntriesLimit();
    void testMultipleCtxGroups();
    void testEmptyEntriesReturnsEmpty();
};

#endif
