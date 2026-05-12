#ifndef TEST_RULEBASEDANALYZER_H
#define TEST_RULEBASEDANALYZER_H

#include <QObject>
#include <QVector>
#include "dltanalyzerinterface.h"

class TestRuleBasedAnalyzer : public QObject
{
    Q_OBJECT

private:
    QVector<DltAnalyzerInterface::LogEntry> makeSampleEntries() const;
    QVector<DltAnalyzerInterface::LogEntry> makeAutoEntries() const;

private slots:
    void testEmptyEntries();
    void testEmptyQuery();
    void testHelpCommand();
    void testTimelineCommand();
    void testKeywordsCommand();
    void testSummaryCommand();
    void testPatternDetection();
    void testLevelFilterError();
    void testLevelFilterWarn();
    void testLevelFilterInfo();
    void testLevelFilterDebug();
    void testDomainFilterCarPlay();
    void testDomainFilterAndroidAuto();
    void testCombinedFilter();
    void testNoMatch();
    void testSimplifyPayload();
    void testFormatEntryLine();
    void testConfigurationInfo();
};

#endif
