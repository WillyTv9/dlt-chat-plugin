#include "test_llmutils.h"
#include "dltchat/llm_analyzer_interface.h"
#include <QTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace dltchat;

void TestLlmUtils::testExtractIndicesWithTag()
{
    DltLlmAnalyzerInterface analyzer;
    QString text = "The error occurred at [index:1452] and [index:1453]. "
                   "The following [index: 1460] also shows issues.";
    auto indices = analyzer.extractIndicesFromText(text);
    QVERIFY(indices.contains(1452));
    QVERIFY(indices.contains(1453));
    QVERIFY(indices.contains(1460));
    QCOMPARE(indices.size(), 3);
}

void TestLlmUtils::testExtractIndicesPlainNumbers()
{
    DltLlmAnalyzerInterface analyzer;
    QString text = "Log entries 42, 43 and 100 show the problem.";
    auto indices = analyzer.extractIndicesFromText(text);
    QVERIFY(!indices.isEmpty());
    QVERIFY(indices.contains(42));
}

void TestLlmUtils::testExtractIndicesMixed()
{
    DltLlmAnalyzerInterface analyzer;
    // Only [index:N] tags are extracted; plain numbers only as fallback when NO tags present
    QString text = "[index:100] First error, then [index:200] second.";
    auto indices = analyzer.extractIndicesFromText(text);
    QVERIFY(indices.contains(100));
    QVERIFY(indices.contains(200));
    QCOMPARE(indices.size(), 2);
}

void TestLlmUtils::testParseOpenaiResponse()
{
    DltLlmAnalyzerInterface analyzer;
    analyzer.setApiEndpoint("https://api.openai.com/v1/chat/completions");

    QJsonObject msg;
    msg["role"] = "assistant";
    msg["content"] = "The service restarted at [index:1452] due to a timeout.";

    QJsonObject choice;
    choice["message"] = msg;
    choice["index"] = 0;

    QJsonObject root;
    root["choices"] = QJsonArray{choice};
    root["model"] = "gpt-4";

    QString json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    QString parsed = analyzer.parseLlmResponse(json);
    QVERIFY(parsed.contains("restarted"));
    QVERIFY(parsed.contains("[index:1452]"));
}

void TestLlmUtils::testParseOllamaResponse()
{
    DltLlmAnalyzerInterface analyzer;
    analyzer.setApiEndpoint("http://localhost:11434/api/generate");

    QJsonObject root;
    root["model"] = "qwen2.5:0.5b";
    root["response"] = "The CAN bus error at [index:300] shows a timeout.";
    root["done"] = true;

    QString json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    QString parsed = analyzer.parseLlmResponse(json);
    QVERIFY(parsed.contains("CAN bus error"));
    QVERIFY(parsed.contains("[index:300]"));
}

void TestLlmUtils::testParsePlainTextResponse()
{
    DltLlmAnalyzerInterface analyzer;
    QString text = "This is a plain text response without JSON formatting.";
    QString parsed = analyzer.parseLlmResponse(text);
    QCOMPARE(parsed, text);
}

void TestLlmUtils::testBuildPrompt()
{
    DltLlmAnalyzerInterface analyzer;
    QVector<DltAnalyzerInterface::LogEntry> entries;

    for (int i = 0; i < 5; ++i) {
        DltAnalyzerInterface::LogEntry e;
        e.index = i; e.time = "10:00:00.000"; e.level = "info";
        e.apid = "APP"; e.ctid = "CTX"; e.payload = "test";
        e.domain = "generic";
        entries.append(e);
    }

    analyzer.setExtraContext("");
    QString prompt = analyzer.buildPrompt("What happened?", entries, 5);
    // New prompt includes system prompt, log section, and query
    QVERIFY(prompt.contains("[0]"));
    QVERIFY(prompt.contains("APP/CTX"));
    QVERIFY(prompt.contains("What happened?"));
    QVERIFY(prompt.contains("LOG ("));
    QVERIFY(prompt.contains("User query:"));

    analyzer.setExtraContext("User filter: CAN errors active");
    prompt = analyzer.buildPrompt("Show errors", entries, 3);
    QVERIFY(prompt.contains("Additional context"));
    QVERIFY(prompt.contains("User filter"));
}

void TestLlmUtils::testAutomotiveSystemPrompt()
{
    QString prompt = DltLlmAnalyzerInterface::buildAutomotiveSystemPrompt();
    QVERIFY(prompt.contains("Automotive SRE"));
    QVERIFY(prompt.contains("AUTOSAR DLT"));
    QVERIFY(prompt.contains("SOME/IP"));
    QVERIFY(prompt.contains("root cause"));
    QVERIFY(prompt.contains("RCA"));
    QVERIFY(prompt.contains("[index:N]"));
    QVERIFY(prompt.contains("CAN bus"));
}

void TestLlmUtils::testEnhancedPromptWithHistory()
{
    DltLlmAnalyzerInterface analyzer;
    ConversationManager convMgr;
    convMgr.addTurn("user", "Show errors");
    convMgr.addTurn("assistant", "Found timeout in ECU1");

    analyzer.setConversationManager(&convMgr);

    QVector<DltAnalyzerInterface::LogEntry> entries;
    DltAnalyzerInterface::LogEntry e;
    e.index = 0; e.time = "10:00:00.000"; e.level = "error";
    e.apid = "APP"; e.ctid = "CTX"; e.payload = "timeout detected";
    e.domain = "generic";
    entries.append(e);

    QString prompt = analyzer.buildEnhancedPrompt("Why did it happen?", entries, 5);
    QVERIFY(prompt.contains("Previous conversation:"));
    QVERIFY(prompt.contains("Show errors"));
    QVERIFY(prompt.contains("Found timeout"));
    QVERIFY(prompt.contains("User query: Why did it happen?"));
    QVERIFY(prompt.contains("LOG ("));
    QVERIFY(prompt.contains("timeout detected"));
}

void TestLlmUtils::testEnhancedPromptWithExtraInfo()
{
    DltLlmAnalyzerInterface analyzer;
    QVector<DltAnalyzerInterface::LogEntry> entries;
    DltAnalyzerInterface::LogEntry e;
    e.index = 1; e.time = "10:00:01.000"; e.level = "warn";
    e.apid = "SYS"; e.ctid = "DIAG"; e.payload = "high cpu";
    e.domain = "generic";
    entries.append(e);

    QString prompt = analyzer.buildEnhancedPrompt("CPU issue?", entries, 5, "User filters: CAN");
    QVERIFY(prompt.contains("Additional context"));
    QVERIFY(prompt.contains("User filters: CAN"));
    QVERIFY(prompt.contains("User query: CPU issue?"));
    QVERIFY(prompt.contains("SYS/DIAG"));
}

void TestLlmUtils::testDetectProviderType()
{
    DltLlmAnalyzerInterface analyzer;

    analyzer.setApiEndpoint("https://api.openai.com/v1/chat/completions");
    QCOMPARE(analyzer.detectProviderType(), "openai");

    analyzer.setApiEndpoint("https://my-azure.openai.azure.com");
    QCOMPARE(analyzer.detectProviderType(), "openai");

    analyzer.setApiEndpoint("https://api.anthropic.com/v1/messages");
    QCOMPARE(analyzer.detectProviderType(), "claude");

    analyzer.setApiEndpoint("http://localhost:11434/api/generate");
    QCOMPARE(analyzer.detectProviderType(), "ollama");

    analyzer.setApiEndpoint("http://localhost:8080/v1/completions");
    QCOMPARE(analyzer.detectProviderType(), "openai-compat");
}

void TestLlmUtils::testBuildRequestBody()
{
    DltLlmAnalyzerInterface analyzer;
    analyzer.setApiEndpoint("http://localhost:11434/api/generate");
    analyzer.setModelName("test-model");
    QString prompt = "Test prompt";
    QByteArray body = analyzer.buildRequestBody(prompt);

    QJsonDocument doc = QJsonDocument::fromJson(body);
    QVERIFY(doc.isObject());
    QJsonObject obj = doc.object();
    QCOMPARE(obj["model"].toString(), "test-model");
    // Ollama: prompt should contain system prefix
    QString promptStr = obj["prompt"].toString();
    QVERIFY(promptStr.contains("Automotive SRE"));
    QVERIFY(promptStr.contains("Test prompt"));
    QVERIFY(obj["stream"].isBool());
    QCOMPARE(obj["stream"].toBool(), false);
    QVERIFY(obj.contains("options"));

    analyzer.setApiEndpoint("https://api.openai.com/v1/chat/completions");
    body = analyzer.buildRequestBody(prompt);
    doc = QJsonDocument::fromJson(body);
    obj = doc.object();
    QVERIFY(obj.contains("messages"));
    // OpenAI: messages should contain system prompt
    QJsonArray msgs = obj["messages"].toArray();
    QCOMPARE(msgs.size(), 2);
    QCOMPARE(msgs[0].toObject()["role"].toString(), "system");
    QVERIFY(msgs[0].toObject()["content"].toString().contains("Automotive SRE"));
    QCOMPARE(msgs[1].toObject()["role"].toString(), "user");

    // Test Claude provider
    analyzer.setApiEndpoint("https://api.anthropic.com/v1/messages");
    body = analyzer.buildRequestBody(prompt);
    doc = QJsonDocument::fromJson(body);
    obj = doc.object();
    QVERIFY(obj.contains("system"));
    QVERIFY(obj["system"].toString().contains("Automotive SRE"));
    QVERIFY(obj.contains("messages"));
}
