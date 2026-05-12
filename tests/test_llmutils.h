#ifndef TEST_LLMUTILS_H
#define TEST_LLMUTILS_H

#include <QObject>

class TestLlmUtils : public QObject
{
    Q_OBJECT

private slots:
    void testExtractIndicesWithTag();
    void testExtractIndicesPlainNumbers();
    void testExtractIndicesMixed();
    void testParseOpenaiResponse();
    void testParseOllamaResponse();
    void testParsePlainTextResponse();
    void testBuildPrompt();
    void testBuildRequestBody();
};

#endif
