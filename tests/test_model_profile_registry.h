#ifndef TEST_MODEL_PROFILE_REGISTRY_H
#define TEST_MODEL_PROFILE_REGISTRY_H

#include <QObject>

class TestModelProfileRegistry : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testCopilotGpt4o();
    void testOllamaLlama3();
    void testUnknownProvider();
    void testCaseInsensitive();
    void testIsKnown();
};

#endif
