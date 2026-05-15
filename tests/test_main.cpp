#include <QTest>
#include <QCoreApplication>
#include <QVector>
#include <QStringList>
#include "test_rulebasedanalyzer.h"
#include "test_automotivelogparser.h"
#include "test_contextualextractor.h"
#include "test_conversationmanager.h"
#include "test_temporalcorrelator.h"
#include "test_fibexenricher.h"
#include "test_llmutils.h"
#include "test_dltexport.h"
#include "test_userfiltermanager.h"

static int runTestSuite(QObject *testObject)
{
    const QStringList argList = QCoreApplication::arguments();
    QVector<QByteArray> argStorage;
    argStorage.reserve(argList.size());
    for (const QString &arg : argList)
        argStorage.append(arg.toLocal8Bit());

    QVector<char *> argValues;
    argValues.reserve(argStorage.size() + 1);
    for (QByteArray &arg : argStorage)
        argValues.append(arg.data());
    argValues.append(nullptr);

    return QTest::qExec(testObject, argValues.size() - 1, argValues.data());
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;
    TestRuleBasedAnalyzer ruleBased;
    status |= runTestSuite(&ruleBased);

    TestAutomotiveLogParser automotiveLogParser;
    status |= runTestSuite(&automotiveLogParser);

    TestContextualExtractor contextualExtractor;
    status |= runTestSuite(&contextualExtractor);

    TestConversationManager conversationManager;
    status |= runTestSuite(&conversationManager);

    TestTemporalCorrelator temporalCorrelator;
    status |= runTestSuite(&temporalCorrelator);

    TestFibexEnricher fibexEnricher;
    status |= runTestSuite(&fibexEnricher);

    TestLlmUtils llmUtils;
    status |= runTestSuite(&llmUtils);

    TestDltExport dltExport;
    status |= runTestSuite(&dltExport);

    TestUserFilterManager userFilterManager;
    status |= runTestSuite(&userFilterManager);

    return status;
}
