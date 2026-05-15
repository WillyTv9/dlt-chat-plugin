#include <QTest>
#include <QCoreApplication>
#include "test_rulebasedanalyzer.h"
#include "test_automotivelogparser.h"
#include "test_contextualextractor.h"
#include "test_conversationmanager.h"
#include "test_temporalcorrelator.h"
#include "test_fibexenricher.h"
#include "test_llmutils.h"
#include "test_dltexport.h"
#include "test_userfiltermanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;
    TestRuleBasedAnalyzer ruleBased;
    status |= QTest::qExec(&ruleBased, argc, argv);

    TestAutomotiveLogParser automotiveLogParser;
    status |= QTest::qExec(&automotiveLogParser, argc, argv);

    TestContextualExtractor contextualExtractor;
    status |= QTest::qExec(&contextualExtractor, argc, argv);

    TestConversationManager conversationManager;
    status |= QTest::qExec(&conversationManager, argc, argv);

    TestTemporalCorrelator temporalCorrelator;
    status |= QTest::qExec(&temporalCorrelator, argc, argv);

    TestFibexEnricher fibexEnricher;
    status |= QTest::qExec(&fibexEnricher, argc, argv);

    TestLlmUtils llmUtils;
    status |= QTest::qExec(&llmUtils, argc, argv);

    TestDltExport dltExport;
    status |= QTest::qExec(&dltExport, argc, argv);

    TestUserFilterManager userFilterManager;
    status |= QTest::qExec(&userFilterManager, argc, argv);

    return status;
}
