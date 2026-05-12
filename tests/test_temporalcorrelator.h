#ifndef TEST_TEMPORALCORRELATOR_H
#define TEST_TEMPORALCORRELATOR_H

#include <QObject>

class TestTemporalCorrelator : public QObject
{
    Q_OBJECT

private slots:
    void testNoEntriesReturnsEmpty();
    void testSingleEntryReturnsEmpty();
    void testTwoEcusInWindow();
    void testNoCorrelationWhenEcusDistant();
    void testParseTimestampSeconds();
    void testParseTimestampTimeFormat();
    void testParseTimestampPlainNumber();
};

#endif
