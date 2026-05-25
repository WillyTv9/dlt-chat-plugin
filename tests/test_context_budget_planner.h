#ifndef TEST_CONTEXT_BUDGET_PLANNER_H
#define TEST_CONTEXT_BUDGET_PLANNER_H

#include <QObject>

class TestContextBudgetPlanner : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testGlobalQueryDetection();
    void testEnglishGlobalDetection();
    void testDeepPrefix();
    void testPlanForCopilot();
    void testPlanForGlobalQuery();
    void testPlanForOllama();
};

#endif
