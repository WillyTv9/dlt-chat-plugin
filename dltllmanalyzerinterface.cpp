#include "dltllmanalyzerinterface.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>

static constexpr int kLlmMaxEntries = 100;
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

DltLlmAnalyzerInterface::DltLlmAnalyzerInterface(QObject *parent)
    : QObject(parent)
    , m_maxTokens(1000), m_temperature(0.3), m_timeout(30000)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

DltLlmAnalyzerInterface::~DltLlmAnalyzerInterface() {}

bool DltLlmAnalyzerInterface::validateConfiguration() const
{
    if (m_apiEndpoint.isEmpty()) return false;
    bool local = m_apiEndpoint.contains("localhost") || m_apiEndpoint.contains("127.0.0.1")
        || m_apiEndpoint.contains("ollama") || m_apiEndpoint.contains("local-ai");
    if (local) return !m_modelName.isEmpty();
    return !m_apiKey.isEmpty() && !m_modelName.isEmpty();
}

bool DltLlmAnalyzerInterface::isAvailable() const
{
    if (!validateConfiguration()) return false;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastAvailabilityCheck < AVAILABILITY_TTL_MS)
        return m_availabilityVerified;

    QUrl url(m_apiEndpoint);
    QString tagsUrl = url.toString();
    tagsUrl.replace("/api/generate", "/api/tags");

    QNetworkAccessManager mgr;
    QNetworkReply *reply = mgr.get(QNetworkRequest(QUrl(tagsUrl)));
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(3000);
    loop.exec();

    m_lastAvailabilityCheck = now;
    if (timer.isActive() && reply->error() == QNetworkReply::NoError)
    {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject())
        {
            QJsonArray models = doc.object()["models"].toArray();
            for (const auto &m : models)
            {
                if (m.toObject()["name"].toString().startsWith(m_modelName))
                {
                    m_availabilityVerified = true;
                    reply->deleteLater();
                    return true;
                }
            }
        }
    }
    m_availabilityVerified = false;
    reply->deleteLater();
    return false;
}

bool DltLlmAnalyzerInterface::testConnection(QString *errMsg)
{
    if (m_apiEndpoint.isEmpty())
    {
        if (errMsg) *errMsg = "Endpoint non impostato";
        emit connectionTestResult(false, "Endpoint non impostato");
        return false;
    }

    QString tagsUrl = QUrl(m_apiEndpoint).toString().replace("/api/generate", "/api/tags");
    QNetworkAccessManager mgr;
    QNetworkReply *reply = mgr.get(QNetworkRequest(QUrl(tagsUrl)));

    QEventLoop loop;
    QTimer timer; timer.setSingleShot(true); timer.setInterval(5000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(); loop.exec();

    bool ok = timer.isActive() && reply->error() == QNetworkReply::NoError;
    QString msg = ok ? "Connessione OK" : (timer.isActive() ? reply->errorString() : "Timeout");
    reply->deleteLater();
    if (errMsg) *errMsg = msg;
    emit connectionTestResult(ok, msg);
    return ok;
}

QString DltLlmAnalyzerInterface::buildPrompt(const QString &query,
                                             const QVector<LogEntry> &entries,
                                             int maxEntries) const
{
    QStringList ctx;
    int n = qMin(entries.size(), maxEntries);
    ctx.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        const auto &e = entries[i];
        ctx.append(QString("[%1] %2 %3 %4/%5 - %6")
            .arg(e.index).arg(e.time).arg(e.level.toUpper())
            .arg(e.apid).arg(e.ctid).arg(e.payload));
    }
    return QString(
        "You analyze DLT logs. Reference entries as [index:N].\n"
        "Identify patterns, anomalies, error chains, root causes.\n"
        "Answer concisely in the user's language.\n\n"
        "LOG (%1 shown):\n%2\n\nQUERY: %3\n\nANSWER:"
    ).arg(entries.size()).arg(ctx.join("\n")).arg(query);
}

QByteArray DltLlmAnalyzerInterface::buildRequestBody(const QString &prompt) const
{
    QJsonObject json;
    json["model"] = m_modelName;
    json["stream"] = false;

    bool isOpenAI = m_apiEndpoint.contains("openai.com") || m_apiEndpoint.contains("azure");
    if (isOpenAI)
    {
        QJsonArray msgs;
        QJsonObject s; s["role"] = "system"; s["content"] = "You analyze DLT logs. Be concise.";
        QJsonObject u; u["role"] = "user"; u["content"] = prompt;
        msgs.append(s); msgs.append(u);
        json["messages"] = msgs;
        if (m_maxTokens > 0) { json["max_tokens"] = m_maxTokens; json["temperature"] = m_temperature; }
    }
    else
    {
        json["prompt"] = prompt;
        if (m_maxTokens > 0)
            json["options"] = QJsonObject{{"num_predict", m_maxTokens}, {"temperature", m_temperature}};
    }
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

DltAnalyzerInterface::QueryResult DltLlmAnalyzerInterface::processReply(
    QNetworkReply *reply, const QElapsedTimer &timer)
{
    QueryResult r;
    r.usedAi = true;
    r.processingTimeMs = timer.elapsed();

    if (reply->error() != QNetworkReply::NoError)
    {
        r.responseHtml = "Errore AI: " + reply->errorString();
        r.success = false;
        return r;
    }

    QByteArray data = reply->readAll();
    if (data.isEmpty())
    {
        r.responseHtml = "Risposta AI vuota.";
        r.success = false;
        return r;
    }

    QString text = parseLlmResponse(QString::fromUtf8(data));
    r.responseHtml = text;
    r.indices = extractIndicesFromText(text);
    r.success = true;
    return r;
}

void DltLlmAnalyzerInterface::analyzeQueryAsync(const QString &query,
                                                  const QVector<LogEntry> &entries)
{
    if (entries.isEmpty())
    {
        QueryResult r; r.responseHtml = "Nessun log da analizzare."; r.success = false; r.usedAi = true;
        emit queryResultReady(r, query);
        return;
    }

    int maxEntries = qMin(entries.size(), kLlmMaxEntries);
    QString prompt = buildPrompt(query, entries, maxEntries);

    QUrl url(m_apiEndpoint);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty())
        req.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    req.setTransferTimeout(m_timeout);
#endif

    QByteArray body = buildRequestBody(prompt);
    QNetworkReply *reply = m_networkManager->post(req, body);

    QElapsedTimer *timer = new QElapsedTimer();
    timer->start();
    QString *queryCopy = new QString(query);

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, queryCopy]() {
        QueryResult r = processReply(reply, *timer);
        emit queryResultReady(r, *queryCopy);
        reply->deleteLater();
        delete timer;
        delete queryCopy;
    });
}

QString DltLlmAnalyzerInterface::parseLlmResponse(const QString &response) const
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return response.trimmed();

    bool isOpenAI = m_apiEndpoint.contains("openai.com") || m_apiEndpoint.contains("azure");
    if (doc.isObject())
    {
        QJsonObject o = doc.object();
        if (isOpenAI && o.contains("choices"))
        {
            QJsonArray a = o["choices"].toArray();
            if (!a.isEmpty())
            {
                QJsonObject c = a[0].toObject();
                if (c.contains("message")) return c["message"].toObject()["content"].toString().trimmed();
                if (c.contains("text")) return c["text"].toString().trimmed();
            }
        }
        if (o.contains("response")) return o["response"].toString().trimmed();
        if (o.contains("text")) return o["text"].toString().trimmed();
    }
    return response.trimmed();
}

QList<int> DltLlmAnalyzerInterface::extractIndicesFromText(const QString &text) const
{
    QList<int> indices;
    QRegularExpression re("\\[index:\\s*(\\d+)\\]", QRegularExpression::CaseInsensitiveOption);
    auto it = re.globalMatch(text);
    while (it.hasNext()) indices.append(it.next().captured(1).toInt());

    if (indices.isEmpty())
    {
        QRegularExpression plain("\\b(\\d+)\\b");
        auto pit = plain.globalMatch(text);
        while (pit.hasNext() && indices.size() < 20)
        {
            int n = pit.next().captured(1).toInt();
            if (n < 10000 && !indices.contains(n)) indices.append(n);
        }
    }
    return indices;
}

DltAnalyzerInterface::QueryResult DltLlmAnalyzerInterface::analyzeQuery(
    const QString &query, const QVector<LogEntry> &entries)
{
    QElapsedTimer timer; timer.start();
    QueryResult r; r.usedAi = true;

    if (entries.isEmpty())
    {
        r.responseHtml = "Nessun log caricato."; r.processingTimeMs = timer.elapsed();
        return r;
    }

    int n = qMin(entries.size(), 100);
    QString prompt = buildPrompt(query, entries, n);

    QUrl url(m_apiEndpoint);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty())
        req.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    req.setTransferTimeout(m_timeout);
#endif

    QByteArray body = buildRequestBody(prompt);
    QNetworkReply *reply = m_networkManager->post(req, body);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeoutTimer.start(m_timeout);
    loop.exec();

    if (!timeoutTimer.isActive())
    {
        reply->abort();
        r.responseHtml = "Timeout AI. Fallback locale.";
        r.success = false; r.usedAi = false;
        r.processingTimeMs = timer.elapsed();
        reply->deleteLater();
        return r;
    }
    timeoutTimer.stop();

    r = processReply(reply, timer);
    reply->deleteLater();

    r.snippets.reserve(r.indices.size());
    QSet<int> seen;
    for (int idx : r.indices)
    {
        if (seen.contains(idx)) continue;
        seen.insert(idx);
        for (const auto &e : entries)
            if (e.index == idx) { r.snippets.append(e.payload.left(120)); break; }
    }
    return r;
}

QString DltLlmAnalyzerInterface::configurationInfo() const
{
    if (!validateConfiguration())
        return "LLM: non configurato.";
    return QString("LLM: %1 @ %2 | tokens=%3 temp=%4 timeout=%5ms")
        .arg(m_modelName).arg(m_apiEndpoint).arg(m_maxTokens).arg(m_temperature).arg(m_timeout);
}

QStringList DltLlmAnalyzerInterface::supportedLanguages() const { return {"en","it","de","es","fr","zh","ja"}; }

bool DltLlmAnalyzerInterface::configure(const QVariantMap &c)
{
    if (c.contains("apiEndpoint")) setApiEndpoint(c["apiEndpoint"].toString());
    if (c.contains("apiKey")) setApiKey(c["apiKey"].toString());
    if (c.contains("modelName")) setModelName(c["modelName"].toString());
    if (c.contains("maxTokens")) setMaxTokens(c["maxTokens"].toInt());
    if (c.contains("temperature")) setTemperature(c["temperature"].toDouble());
    if (c.contains("timeout")) setTimeout(c["timeout"].toInt());
    return true;
}

QVariantMap DltLlmAnalyzerInterface::currentConfiguration() const
{
    QVariantMap c;
    c["type"] = "llm"; c["apiEndpoint"] = m_apiEndpoint; c["modelName"] = m_modelName;
    c["maxTokens"] = m_maxTokens; c["temperature"] = m_temperature; c["timeout"] = m_timeout;
    c["hasApiKey"] = !m_apiKey.isEmpty();
    return c;
}

void DltLlmAnalyzerInterface::setApiEndpoint(const QString &v)
{ if (m_apiEndpoint != v) { m_apiEndpoint = v; m_lastAvailabilityCheck = 0; emit apiEndpointChanged(v); } }
void DltLlmAnalyzerInterface::setApiKey(const QString &v)
{ if (m_apiKey != v) { m_apiKey = v; m_lastAvailabilityCheck = 0; emit apiKeyChanged(v); } }
void DltLlmAnalyzerInterface::setModelName(const QString &v)
{ if (m_modelName != v) { m_modelName = v; m_lastAvailabilityCheck = 0; emit modelNameChanged(v); } }
void DltLlmAnalyzerInterface::setMaxTokens(int v) { if (m_maxTokens != v) { m_maxTokens = v; emit maxTokensChanged(v); } }
void DltLlmAnalyzerInterface::setTemperature(double v) { if (qAbs(m_temperature - v) > 0.001) { m_temperature = v; emit temperatureChanged(v); } }
void DltLlmAnalyzerInterface::setTimeout(int v) { if (m_timeout != v) { m_timeout = v; emit timeoutChanged(v); } }

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createOpenAIAnalyzer(
    const QString &key, const QString &model, QObject *p)
{
    auto *a = new DltLlmAnalyzerInterface(p);
    a->setApiEndpoint("https://api.openai.com/v1/chat/completions");
    a->setApiKey(key); a->setModelName(model); a->setMaxTokens(1000); a->setTemperature(0.3);
    return a;
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createOllamaAnalyzer(
    const QString &base, const QString &model, QObject *p)
{
    auto *a = new DltLlmAnalyzerInterface(p);
    a->setApiEndpoint(base + "/api/generate");
    a->setApiKey(QString()); a->setModelName(model); a->setMaxTokens(1000); a->setTemperature(0.3);
    return a;
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createLocalAiAnalyzer(
    const QString &base, const QString &model, QObject *p)
{
    auto *a = new DltLlmAnalyzerInterface(p);
    a->setApiEndpoint(base + "/api/generate");
    a->setApiKey(QString()); a->setModelName(model); a->setMaxTokens(1000); a->setTemperature(0.3);
    return a;
}

QStringList DltLlmAnalyzerFactory::availableProviders() { return {"openai", "ollama", "local-ai"}; }
QString DltLlmAnalyzerFactory::defaultModelForProvider(const QString &p)
{
    if (p == "openai") return "gpt-4";
    if (p == "ollama") return "llama3";
    if (p == "local-ai") return "mistral";
    return "llama3";
}
