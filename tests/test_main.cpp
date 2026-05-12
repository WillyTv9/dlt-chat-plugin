#include <QTest>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include "test_rulebasedanalyzer.h"
#include "test_automotivelogparser.h"
#include "test_contextualextractor.h"
#include "test_conversationmanager.h"
#include "test_temporalcorrelator.h"
#include "test_fibexenricher.h"
#include "test_llmutils.h"
#include "test_dltexport.h"
#include "test_userfiltermanager.h"
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    TestRuleBasedAnalyzer t1; TestAutomotiveLogParser t2; TestContextualExtractor t3;
    TestConversationManager t3b; TestTemporalCorrelator t3c; TestFibexEnricher t3d; TestLlmUtils t4;
    TestDltExport t5; TestUserFilterManager t6;
    struct { const char *n; QObject *o; } ts[] = {
        {"RBA", &t1}, {"ALP", &t2}, {"CE", &t3}, {"CM", &t3b},
        {"TC", &t3c}, {"FE", &t3d}, {"LLM", &t4}, {"DE", &t5}, {"UFM", &t6}
    };
    int total = 0; QFile f("r.txt"); f.open(QIODevice::WriteOnly|QIODevice::Text); QTextStream o(&f);
    for (auto &e : ts) { int r = QTest::qExec(e.o, argc, argv); o << e.n << ": " << r << "\n"; total += r; }
    o << "TOTAL: " << total << "\n"; f.close(); return total;
}
