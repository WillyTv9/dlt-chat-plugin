#include "test_llmutils.h"
#include "dltllmanalyzerinterface.h"
#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>

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
    QString text = "[index:100] First error, then [index:200] second. Number 300 also relevant.";
    auto indices = analyzer.extractIndicesFromText(text);
    QVERIFY(indices.contains(100));
    QVERIFY(indices.contains(200));
    QCOMPARE(indices.size(), 3);
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
    QVERIFY(prompt.contains("[0]"));
    QVERIFY(prompt.contains("APP/CTX"));
    QVERIFY(prompt.contains("What happened?"));
    QVERIFY(prompt.contains("[index:N]"));

    analyzer.setExtraContext("User filter: CAN errors active");
    prompt = analyzer.buildPrompt("Show errors", entries, 3);
    QVERIFY(prompt.contains("Additional context"));
    QVERIFY(prompt.contains("User filter"));
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
    QCOMPARE(obj["prompt"].toString(), "Test prompt");
    QVERIFY(obj["stream"].isBool());
    QCOMPARE(obj["stream"].toBool(), false);
    QVERIFY(obj.contains("options"));

    analyzer.setApiEndpoint("https://api.openai.com/v1/chat/completions");
    body = analyzer.buildRequestBody(prompt);
    doc = QJsonDocument::fromJson(body);
    obj = doc.object();
    QVERIFY(obj.contains("messages"));
}
