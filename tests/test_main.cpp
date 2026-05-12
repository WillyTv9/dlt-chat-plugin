#include <QTest>
#include <QCoreApplication>
#include "test_rulebasedanalyzer.h"
#include "test_automotivelogparser.h"
#include "test_llmutils.h"
#include "test_dltexport.h"
#include "test_userfiltermanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int failures = 0;

    TestRuleBasedAnalyzer t1;
    failures += QTest::qExec(&t1, argc, argv);

    TestAutomotiveLogParser t2;
    failures += QTest::qExec(&t2, argc, argv);

    TestLlmUtils t3;
    failures += QTest::qExec(&t3, argc, argv);

    TestDltExport t4;
    failures += QTest::qExec(&t4, argc, argv);

    TestUserFilterManager t5;
    failures += QTest::qExec(&t5, argc, argv);

    return failures;
}
