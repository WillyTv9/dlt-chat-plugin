#ifndef TEST_RULEBASEDANALYZER_H
#define TEST_RULEBASEDANALYZER_H

#include <QObject>
#include <QVector>
#include "dltchat/analyzer_interface.h"

class TestRuleBasedAnalyzer : public QObject
{
    Q_OBJECT

private:
    QVector<dltchat::DltAnalyzerInterface::LogEntry> makeSampleEntries() const;
    QVector<dltchat::DltAnalyzerInterface::LogEntry> makeAutoEntries() const;

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
