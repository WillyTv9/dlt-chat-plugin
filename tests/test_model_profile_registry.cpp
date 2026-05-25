#include "test_model_profile_registry.h"

#include "dltchat/model_profile_registry.h"

#include <QTest>

using namespace dltchat;

void TestModelProfileRegistry::init()
{
}

void TestModelProfileRegistry::cleanup()
{
}

void TestModelProfileRegistry::testCopilotGpt4o()
{
    ModelProfile p = ModelProfileRegistry::profileFor("copilot", "gpt-4o");
    QCOMPARE(p.maxContextTokens, 128000);
    QCOMPARE(p.maxConcurrent, 2);
    QVERIFY(!p.isFallback);
    QCOMPARE(p.provider, QString("copilot"));
    QCOMPARE(p.model, QString("gpt-4o"));
    QVERIFY(p.reservedForResponse > 0);
}

void TestModelProfileRegistry::testOllamaLlama3()
{
    ModelProfile p = ModelProfileRegistry::profileFor("ollama", "llama3");
    QCOMPARE(p.maxContextTokens, 8192);
    QVERIFY(!p.isFallback);
    QVERIFY(p.maxConcurrent >= 1);
}

void TestModelProfileRegistry::testUnknownProvider()
{
    ModelProfile p = ModelProfileRegistry::profileFor("xyz_provider", "abc_model");
    QVERIFY(p.isFallback);
    // Conservative defaults from the registry.
    QCOMPARE(p.maxContextTokens, 8192);
    QCOMPARE(p.reservedForResponse, 2048);
    QCOMPARE(p.maxConcurrent, 2);
}

void TestModelProfileRegistry::testCaseInsensitive()
{
    ModelProfile p = ModelProfileRegistry::profileFor("COPILOT", "GPT-4O");
    QCOMPARE(p.maxContextTokens, 128000);
    QVERIFY(!p.isFallback);

    ModelProfile p2 = ModelProfileRegistry::profileFor("  Ollama  ", "  Llama3  ");
    QCOMPARE(p2.maxContextTokens, 8192);
    QVERIFY(!p2.isFallback);
}

void TestModelProfileRegistry::testIsKnown()
{
    QVERIFY(ModelProfileRegistry::isKnown("copilot", "gpt-4o"));
    QVERIFY(ModelProfileRegistry::isKnown("openai", "gpt-4o-mini"));
    QVERIFY(ModelProfileRegistry::isKnown("ollama", "llama3"));
    QVERIFY(ModelProfileRegistry::isKnown("COPILOT", "GPT-4O"));

    QVERIFY(!ModelProfileRegistry::isKnown("xyz_provider", "abc_model"));
    // Empty-prefix generic entries are fallbacks, not "known".
    QVERIFY(!ModelProfileRegistry::isKnown("copilot", "some-unknown-model"));
}
