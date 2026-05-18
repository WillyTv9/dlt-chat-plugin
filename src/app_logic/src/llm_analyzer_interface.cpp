#include "dltchat/llm_analyzer_interface.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QRandomGenerator>
#include <QThread>
#include <algorithm>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace dltchat {

static constexpr int kMaxRetries = 3;
static constexpr int kBaseRetryDelayMs = 1000;

DltLlmAnalyzerInterface::DltLlmAnalyzerInterface(QObject *parent)
    : QObject(parent)
    , m_maxTokens(4096), m_temperature(0.7), m_timeout(120000)
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

    m_lastAvailabilityCheck = now;
    QString provider = detectProviderType();
    QNetworkAccessManager mgr;
    QNetworkReply *reply = nullptr;

    if (provider == "copilot") {
        m_lastAvailabilityCheck = now;
        m_availabilityVerified = true;
        return true;
    } else if (provider == "ollama") {
        QString tagsUrl = QUrl(m_apiEndpoint).toString();
        tagsUrl.replace("/api/generate", "/api/tags").replace("/api/chat", "/api/tags");
        QUrl tagsQUrl(tagsUrl);
        reply = mgr.get(QNetworkRequest(tagsQUrl));
    } else {
        QUrl base(m_apiEndpoint);
        QString modelsUrl = base.scheme() + "://" + base.authority() + "/v1/models";
        QUrl probeUrl(modelsUrl);
        QNetworkRequest req(probeUrl);
        QString authKey = m_apiKey;
        if (!authKey.isEmpty())
            req.setRawHeader("Authorization", QString("Bearer %1").arg(authKey).toUtf8());
        reply = mgr.get(req);
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(5000);
    loop.exec();

    bool ok = false;
    if (timer.isActive()) {
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (provider == "ollama") {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                if (doc.isObject()) {
                    QJsonArray models = doc.object()["models"].toArray();
                    for (const auto &m : models) {
                        if (m.toObject()["name"].toString().contains(m_modelName, Qt::CaseInsensitive)) {
                            ok = true; break;
                        }
                    }
                    if (!ok && !models.isEmpty()) ok = true;
                }
            }
        } else {
            ok = (httpCode >= 200 && httpCode < 300);
        }
    }

    m_availabilityVerified = ok;
    reply->deleteLater();
    return ok;
}

bool DltLlmAnalyzerInterface::testConnection(QString *errMsg)
{
    if (m_apiEndpoint.isEmpty())
    {
        if (errMsg) *errMsg = "Endpoint non impostato";
        emit connectionTestResult(false, "Endpoint non impostato");
        return false;
    }

    QString provider = detectProviderType();
    QNetworkAccessManager mgr;
    QNetworkReply *reply = nullptr;

    if (provider == "ollama") {
        QString tagsUrl = QUrl(m_apiEndpoint).toString();
        tagsUrl.replace("/api/generate", "/api/tags").replace("/api/chat", "/api/tags");
        QUrl tagsQUrl2(tagsUrl);
        reply = mgr.get(QNetworkRequest(tagsQUrl2));
    } else {
        QUrl base(m_apiEndpoint);
        QString modelsUrl = base.scheme() + "://" + base.authority();
        if (provider == "openai" || provider == "openai-compat" || provider == "copilot")
            modelsUrl += "/v1/models";
        QUrl probeUrl2(modelsUrl);
        QNetworkRequest req(probeUrl2);
        // Resolve effective auth key (exchanges Copilot OAuth token if needed)
        QString authKey = resolvedApiKey();
        if (!authKey.isEmpty())
            req.setRawHeader("Authorization", QString("Bearer %1").arg(authKey).toUtf8());
        if (provider == "copilot") {
            req.setRawHeader("Editor-Version", "DLTChatPlugin/1.0");
            req.setRawHeader("Copilot-Integration-Id", "dlt-chat-plugin");
        }
        reply = mgr.get(req);
    }

    QEventLoop loop;
    QTimer timer; timer.setSingleShot(true); timer.setInterval(5000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(); loop.exec();

    bool ok = false;
    QString msg;
    if (!timer.isActive()) {
        msg = "Timeout";
    } else if (provider == "ollama") {
        ok = reply->error() == QNetworkReply::NoError;
        msg = ok ? "Connessione OK (Ollama)" : reply->errorString();
    } else {
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpCode >= 200 && httpCode < 300) {
            ok = true;
            msg = QString("Connessione OK (HTTP %1)").arg(httpCode);
        } else if (httpCode == 401 || httpCode == 403) {
            ok = false;
            msg = QString("Autenticazione fallita (HTTP %1)").arg(httpCode);
        } else if (httpCode > 0) {
            ok = false;
            msg = QString("Errore server (HTTP %1)").arg(httpCode);
        } else {
            ok = false;
            msg = reply->errorString();
        }
    }

    reply->deleteLater();
    // Invalidate cached availability so next isAvailable() re-probes
    m_lastAvailabilityCheck = 0;
    m_availabilityVerified = ok;
    if (errMsg) *errMsg = msg;
    emit connectionTestResult(ok, msg);
    return ok;
}

QString DltLlmAnalyzerInterface::effectiveCopilotBearer() const
{
    if (m_apiKey.isEmpty()) return {};
    bool isRawOAuth = m_apiKey.startsWith("gho_") || m_apiKey.startsWith("ghu_") ||
                      m_apiKey.startsWith("ghp_") || m_apiKey.startsWith("github_pat_");
    if (!isRawOAuth) return m_apiKey;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!m_copilotBearer.isEmpty() && now < m_copilotBearerExpiry - 60000)
        return m_copilotBearer;

    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl("https://api.github.com/copilot_internal/v2/token"));
    req.setRawHeader("Authorization", QString("token %1").arg(m_apiKey).toUtf8());
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Editor-Version", "DLTChatPlugin/1.0");
    QNetworkReply *reply = mgr.get(req);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(8000);
    loop.exec();

    if (!timer.isActive()) { reply->deleteLater(); return m_apiKey; }

    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    reply->deleteLater();

    QString token = obj["token"].toString();
    if (token.isEmpty()) return m_apiKey;

    m_copilotBearer = token;
    QDateTime expiry = QDateTime::fromString(obj["expires_at"].toString(), Qt::ISODate);
    m_copilotBearerExpiry = expiry.isValid() ? expiry.toMSecsSinceEpoch()
                                             : (now + 25LL * 60000);
    return m_copilotBearer;
}

QString DltLlmAnalyzerInterface::resolvedApiKey() const
{
    if (detectProviderType() == "copilot")
        return effectiveCopilotBearer();
    return m_apiKey;
}

QString DltLlmAnalyzerInterface::cachedCopilotBearer() const
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!m_copilotBearer.isEmpty() && now < m_copilotBearerExpiry - 60000)
        return m_copilotBearer;
    return {};
}
QString DltLlmAnalyzerInterface::buildPrompt(const QString &query,
                                             const QVector<LogEntry> &entries,
                                             int maxEntries) const
{
    return buildEnhancedPrompt(query, entries, maxEntries, m_extraContext);
}

QString DltLlmAnalyzerInterface::buildAutomotiveSystemPrompt()
{
    return QString(
        "You are an Automotive SRE (Site Reliability Engineer) specialized in AUTOSAR DLT log analysis.\n"
        "Your role is to act as a colleague providing a reliable second pair of eyes on diagnostic logs.\n\n"
        "## Domain Expertise\n"
        "- AUTOSAR DLT protocol, AppID, CtxID, and payload structures\n"
        "- SOME/IP service discovery and communication timeouts\n"
        "- CAN bus protocol and network topology\n"
        "- ECU state machines and power management\n"
        "- Real-time operating system task scheduling and priorities\n\n"
        "## Analysis Approach\n"
        "1. Identify the root cause, not just symptoms\n"
        "2. Correlate timestamps across ECUs to find communication delays\n"
        "3. Detect state machine inconsistencies\n"
        "4. Determine if errors stem from:\n"
        "   - Network timeouts (SOME/IP)\n"
        "   - CPU saturation on the ECU\n"
        "   - Software logic errors or race conditions\n"
        "   - Physical layer issues (cabling, interference, bus load)\n"
        "5. Reference specific log entries as [index:N]\n\n"
        "## Response Style\n"
        "- Be conversational, direct, and helpful. Do NOT use forced step-by-step reasoning formats (e.g., 'Step 1:', 'Step 2:').\n"
        "- Never end your response with a math-solver conclusion box like '\\boxed{}'.\n"
        "- Be technical and precise: cite protocols, task priorities, buffer overflows when relevant.\n"
        "- Respond in the same language as the user's question.\n"
        "- If the question is simple (like asking for an average or a count), just provide the answer directly without over-analyzing.\n"
        "- Reference specific log entries as [index:N].\n"
        "- If enriched function names from .fibex/.xml are not loaded, mention that loading them would improve precision.\n"
        "- Prioritize Root Cause Analysis over summary only when investigating actual errors."
    );
}

QString DltLlmAnalyzerInterface::detectProviderType() const
{
    if (m_apiEndpoint.contains("githubcopilot.com", Qt::CaseInsensitive))
        return "copilot";
    if (m_apiEndpoint.contains("openai.com", Qt::CaseInsensitive) ||
        m_apiEndpoint.contains("azure.com", Qt::CaseInsensitive))
        return "openai";
    if (m_apiEndpoint.contains("anthropic.com", Qt::CaseInsensitive) ||
        m_apiEndpoint.contains("claude", Qt::CaseInsensitive))
        return "claude";
    if (m_apiEndpoint.contains("ollama", Qt::CaseInsensitive) ||
        m_apiEndpoint.contains("11434", Qt::CaseInsensitive))
        return "ollama";
    return "openai-compat";
}

QString DltLlmAnalyzerInterface::buildEnhancedPrompt(
    const QString &query,
    const QVector<LogEntry> &entries,
    int maxEntries,
    const QString &extraInfo) const
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

    QString historyText;
    if (m_conversationManager && !m_conversationManager->isEmpty())
        historyText = m_conversationManager->formatHistory(3);

    QString fibexNote;
    if (!m_fibexLoaded)
        fibexNote = "\nNote: .fibex/.xml enrichment files not loaded. "
                    "Function IDs appear as raw identifiers. "
                    "Load fibex files for more precise analysis.";

    QString extra = extraInfo.isEmpty() ? QString() :
        QString("\nAdditional context:\n%1\n").arg(extraInfo);

    QStringList sections;

    if (!historyText.isEmpty())
        sections << historyText;

    sections << QString("LOG (%1 entries shown, sorted by timestamp):\n%2")
        .arg(entries.size()).arg(ctx.join("\n"));

    if (!fibexNote.isEmpty())
        sections << fibexNote;

    if (!extra.isEmpty())
        sections << extra;

    sections << QString("User query: %1").arg(query);

    return sections.join("\n\n");
}

QByteArray DltLlmAnalyzerInterface::buildRequestBody(const QString &prompt) const
{
    QJsonObject json;
    json["model"] = m_modelName;
    json["stream"] = false;

    QString provider = detectProviderType();

    if (provider == "openai")
    {
        QJsonArray msgs;
        QJsonObject s; s["role"] = "system"; s["content"] = buildAutomotiveSystemPrompt();
        QJsonObject u; u["role"] = "user"; u["content"] = prompt;
        msgs.append(s); msgs.append(u);
        json["messages"] = msgs;
        if (m_maxTokens > 0) { json["max_tokens"] = m_maxTokens; json["temperature"] = m_temperature; }
    }
    else if (provider == "claude")
    {
        QJsonArray msgs;
        QJsonObject u; u["role"] = "user"; u["content"] = prompt;
        msgs.append(u);
        json["messages"] = msgs;
        json["system"] = buildAutomotiveSystemPrompt();
        if (m_maxTokens > 0) { json["max_tokens"] = m_maxTokens; }
        json["temperature"] = m_temperature;
    }
    else if (provider == "copilot")
    {
        QJsonArray msgs;
        QJsonObject s; s["role"] = "system"; s["content"] = buildAutomotiveSystemPrompt();
        QJsonObject u; u["role"] = "user"; u["content"] = prompt;
        msgs.append(s); msgs.append(u);
        json["messages"] = msgs;
        if (m_maxTokens > 0) { json["max_tokens"] = m_maxTokens; json["temperature"] = m_temperature; }
    }
    else if (provider == "ollama")
    {
        QJsonArray msgs;
        QJsonObject s; s["role"] = "system"; s["content"] = buildAutomotiveSystemPrompt();
        QJsonObject u; u["role"] = "user"; u["content"] = prompt;
        msgs.append(s); msgs.append(u);
        json["messages"] = msgs;
        if (m_maxTokens > 0)
            json["options"] = QJsonObject{{"num_predict", m_maxTokens}, {"temperature", m_temperature}};
    }
    else
    {
        // openai-compat: use messages format
        QJsonArray msgs;
        QJsonObject s; s["role"] = "system"; s["content"] = buildAutomotiveSystemPrompt();
        QJsonObject u; u["role"] = "user"; u["content"] = prompt;
        msgs.append(s); msgs.append(u);
        json["messages"] = msgs;
        if (m_maxTokens > 0) { json["max_tokens"] = m_maxTokens; json["temperature"] = m_temperature; }
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

QString DltLlmAnalyzerInterface::parseLlmResponse(const QString &response) const
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return response.trimmed();

    bool isOpenAI = m_apiEndpoint.contains("openai.com") || m_apiEndpoint.contains("azure")
                    || m_apiEndpoint.contains("githubcopilot.com");
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
    std::sort(indices.begin(), indices.end());
    return indices;
}

static QString buildCacheKey(const QString &query, const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(query.toUtf8());
    int n = qMin(entries.size(), 20);
    for (int i = 0; i < n; ++i) {
        hash.addData(QByteArray::number(entries[i].index));
    }
    hash.addData(QByteArray::number(entries.size()));
    return hash.result().toHex();
}

bool DltLlmAnalyzerInterface::analyzeQueryAsync(const QString &query,
                                                 const QVector<LogEntry> &entries)
{
    QMutexLocker lock(&m_circuitMutex);
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_circuitState == CircuitState::Open)
    {
        if (now - m_lastFailureTime > CIRCUIT_OPEN_TIMEOUT_MS)
        {
            m_circuitState = CircuitState::HalfOpen;
        }
        else
        {
            QueryResult r;
            r.responseHtml = "Circuito aperto: troppi fallimenti. Riprova tra un minuto.";
            r.success = false;
            r.usedAi = true;
            emit queryResultReady(r, query);
            return false;
        }
    }
    lock.unlock();

    if (m_conversationManager)
        m_conversationManager->addTurn("user", query);

    if (entries.isEmpty())
    {
        QueryResult r; r.responseHtml = "Nessun log da analizzare."; r.success = false; r.usedAi = true;
        emit queryResultReady(r, query);
        return true;
    }

    QString cacheKey = buildCacheKey(query, entries);
    {
        QMutexLocker lockCache(&m_cacheMutex);
        if (m_responseCache.contains(cacheKey))
        {
            m_cacheAccessOrder.removeAll(cacheKey);
            m_cacheAccessOrder.append(cacheKey);
            CacheEntry entry = m_responseCache.value(cacheKey);
            QueryResult r = entry.result;
            r.processingTimeMs = 0;
            emit queryResultReady(r, query);
            return true;
        }
    }

    QMutexLocker lockRate(&m_rateLimiter.mutex);
    qint64 nowRate = QDateTime::currentMSecsSinceEpoch();
    double elapsedSec = (nowRate - m_rateLimiter.lastRefill) / 1000.0;
    m_rateLimiter.tokens = qMin(m_rateLimiter.capacity,
                                 m_rateLimiter.tokens + elapsedSec * m_rateLimiter.refillRate);
    m_rateLimiter.lastRefill = nowRate;

    if (m_rateLimiter.tokens < 1.0)
    {
        int waitMs = static_cast<int>((1.0 - m_rateLimiter.tokens) / m_rateLimiter.refillRate * 1000);
        lockRate.unlock();

        QTimer::singleShot(waitMs, this, [this, query, entries]() {
            analyzeQueryAsync(query, entries);
        });
        return true;
    }
    m_rateLimiter.tokens -= 1.0;
    lockRate.unlock();

    int maxEntries = qMin(entries.size(), m_maxLogEntries);
    QString prompt = buildEnhancedPrompt(query, entries, maxEntries, m_extraContext);

    QString endpointStr = m_apiEndpoint;
    if (detectProviderType() == "ollama")
        endpointStr.replace("/api/generate", "/api/chat");

    QUrl url(endpointStr);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString provider = detectProviderType();
    if (provider == "copilot") {
        // Copilot: use the OAuth token directly (chat completions accepts it)
        if (!m_apiKey.isEmpty())
            req.setRawHeader("Authorization",
                QString("Bearer %1").arg(m_apiKey).toUtf8());
        req.setRawHeader("Editor-Version", "DLTChatPlugin/1.0");
        req.setRawHeader("Copilot-Integration-Id", "dlt-chat-plugin");
    } else {
        QString authKey = resolvedApiKey();
        if (!authKey.isEmpty())
            req.setRawHeader("Authorization",
                QString("Bearer %1").arg(authKey).toUtf8());
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    req.setTransferTimeout(m_timeout);
#endif

    QByteArray body = buildRequestBody(prompt);
    QNetworkReply *reply = m_networkManager->post(req, body);

    QElapsedTimer *timer = new QElapsedTimer();
    timer->start();
    QString *queryCopy = new QString(query);
    QString *cacheKeyPtr = new QString(cacheKey);

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, queryCopy, cacheKeyPtr]() {
        QueryResult r = processReply(reply, *timer);

        if (r.success && m_conversationManager) {
            m_conversationManager->addTurn("assistant", r.responseHtml.left(500));
        }

        {
            QMutexLocker lock(&m_circuitMutex);
            if (!r.success)
            {
                m_failureCount++;
                m_lastFailureTime = QDateTime::currentMSecsSinceEpoch();
                if (m_failureCount >= CIRCUIT_FAILURE_THRESHOLD)
                {
                    m_circuitState = CircuitState::Open;
                }
            }
            else
            {
                m_failureCount = 0;
                m_circuitState = CircuitState::Closed;
            }
        }

        if (r.success)
        {
            QMutexLocker lock(&m_cacheMutex);
            if (m_responseCache.size() >= CACHE_MAX_SIZE)
            {
                int removeCount = m_cacheAccessOrder.size() / 2;
                for (int i = 0; i < removeCount; ++i)
                {
                    m_responseCache.remove(m_cacheAccessOrder[i]);
                }
                m_cacheAccessOrder.erase(m_cacheAccessOrder.begin(),
                                         m_cacheAccessOrder.begin() + removeCount);
            }
            m_cacheAccessOrder.removeAll(*cacheKeyPtr);
            m_cacheAccessOrder.append(*cacheKeyPtr);
            CacheEntry entry;
            entry.result = r;
            entry.timestamp = QDateTime::currentDateTime();
            m_responseCache[*cacheKeyPtr] = entry;
        }

        emit queryResultReady(r, *queryCopy);
        reply->deleteLater();
        delete timer;
        delete queryCopy;
        delete cacheKeyPtr;
    });

    return true;
}

DltAnalyzerInterface::QueryResult DltLlmAnalyzerInterface::analyzeQuery(
    const QString &query, const QVector<LogEntry> &entries)
{
    QElapsedTimer timer; timer.start();
    QueryResult r; r.usedAi = true;

    {
        QMutexLocker lock(&m_circuitMutex);
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_circuitState == CircuitState::Open)
        {
            if (now - m_lastFailureTime > CIRCUIT_OPEN_TIMEOUT_MS)
            {
                m_circuitState = CircuitState::HalfOpen;
            }
            else
            {
                r.responseHtml = "Circuito aperto: troppi fallimenti. Riprova tra un minuto.";
                r.success = false;
                r.processingTimeMs = timer.elapsed();
                return r;
            }
        }
    }

    if (m_conversationManager)
        m_conversationManager->addTurn("user", query);

    if (entries.isEmpty())
    {
        r.responseHtml = "Nessun log caricato."; r.processingTimeMs = timer.elapsed();
        return r;
    }

    QString cacheKey = buildCacheKey(query, entries);
    {
        QMutexLocker lock(&m_cacheMutex);
        if (m_responseCache.contains(cacheKey))
        {
            m_cacheAccessOrder.removeAll(cacheKey);
            m_cacheAccessOrder.append(cacheKey);
            CacheEntry entry = m_responseCache.value(cacheKey);
            r = entry.result;
            r.processingTimeMs = 0;
            return r;
        }
    }

    {
        QMutexLocker lock(&m_rateLimiter.mutex);
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        double elapsedSec = (now - m_rateLimiter.lastRefill) / 1000.0;
        m_rateLimiter.tokens = qMin(m_rateLimiter.capacity,
                                     m_rateLimiter.tokens + elapsedSec * m_rateLimiter.refillRate);
        m_rateLimiter.lastRefill = now;

        if (m_rateLimiter.tokens < 1.0)
        {
            int waitMs = static_cast<int>((1.0 - m_rateLimiter.tokens) / m_rateLimiter.refillRate * 1000);
            lock.unlock();

            QThread::sleep(waitMs / 1000);

            QMutexLocker lock2(&m_rateLimiter.mutex);
            m_rateLimiter.tokens = 0.0;
        }
        else
        {
            m_rateLimiter.tokens -= 1.0;
        }
    }

    int retryCount = 0;
    while (retryCount <= kMaxRetries)
    {
        if (retryCount > 0)
        {
            int baseDelay = kBaseRetryDelayMs * (1 << (retryCount - 1));
            int jitter = QRandomGenerator::global()->bounded(0, baseDelay / 2);
            int delay = baseDelay + jitter;
            QThread::sleep(delay / 1000);
        }

        int n = qMin(entries.size(), m_maxLogEntries);
        QString prompt = buildEnhancedPrompt(query, entries, n, m_extraContext);

        QString endpointStr = m_apiEndpoint;
        if (detectProviderType() == "ollama")
            endpointStr.replace("/api/generate", "/api/chat");

        QUrl url(endpointStr);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!m_apiKey.isEmpty())
            req.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
        if (detectProviderType() == "copilot") {
            req.setRawHeader("Editor-Version", "DLTChatPlugin/1.0");
            req.setRawHeader("Copilot-Integration-Id", "dlt-chat-plugin");
        }

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
            reply->deleteLater();
            retryCount++;
            continue;
        }
        timeoutTimer.stop();

        r = processReply(reply, timer);
        if (r.success && m_conversationManager) {
            m_conversationManager->addTurn("assistant", r.responseHtml.left(500));
        }
        reply->deleteLater();

        if (r.success)
        {
            {
                QMutexLocker lock(&m_cacheMutex);
                if (m_responseCache.size() >= CACHE_MAX_SIZE)
                {
                    int removeCount = m_cacheAccessOrder.size() / 2;
                    for (int i = 0; i < removeCount; ++i)
                    {
                        m_responseCache.remove(m_cacheAccessOrder[i]);
                    }
                    m_cacheAccessOrder.erase(m_cacheAccessOrder.begin(),
                                             m_cacheAccessOrder.begin() + removeCount);
                }
                m_cacheAccessOrder.removeAll(cacheKey);
                m_cacheAccessOrder.append(cacheKey);
                CacheEntry entry;
                entry.result = r;
                entry.timestamp = QDateTime::currentDateTime();
                m_responseCache[cacheKey] = entry;
            }

            {
                QMutexLocker lock(&m_circuitMutex);
                m_failureCount = 0;
                m_circuitState = CircuitState::Closed;
            }

            break;
        }
        else
        {
            QMutexLocker lock(&m_circuitMutex);
            m_failureCount++;
            m_lastFailureTime = QDateTime::currentMSecsSinceEpoch();
            if (m_failureCount >= CIRCUIT_FAILURE_THRESHOLD)
            {
                m_circuitState = CircuitState::Open;
                r.responseHtml = "Circuito aperto dopo ripetuti fallimenti.";
                return r;
            }
            lock.unlock();

            retryCount++;
            if (retryCount > kMaxRetries)
            {
                r.responseHtml = "Timeout AI. Fallback locale.";
                r.success = false;
                r.usedAi = false;
            }
        }
    }

    r.snippets.reserve(r.indices.size());
    QSet<int> seen;
    for (int idx : r.indices)
    {
        if (seen.contains(idx)) continue;
        seen.insert(idx);
        for (const auto &e : entries)
            if (e.index == idx) { r.snippets.append(e.payload.left(120)); break; }
    }

    r.processingTimeMs = timer.elapsed();
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
    if (c.contains("maxLogEntries")) setMaxLogEntries(c["maxLogEntries"].toInt());
    return true;
}

QVariantMap DltLlmAnalyzerInterface::currentConfiguration() const
{
    QVariantMap c;
    c["type"] = "llm"; c["apiEndpoint"] = m_apiEndpoint; c["modelName"] = m_modelName;
    c["maxTokens"] = m_maxTokens; c["temperature"] = m_temperature; c["timeout"] = m_timeout;
    c["hasApiKey"] = !m_apiKey.isEmpty();
    c["maxLogEntries"] = m_maxLogEntries;
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
void DltLlmAnalyzerInterface::setMaxLogEntries(int v) { if (m_maxLogEntries != v) { m_maxLogEntries = qMax(1, v); emit maxLogEntriesChanged(m_maxLogEntries); } }

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createOpenAIAnalyzer(
    const QString &key, const QString &model, QObject *p)
{
    auto *a = new DltLlmAnalyzerInterface(p);
    a->setApiEndpoint("https://api.openai.com/v1/chat/completions");
    a->setApiKey(key); a->setModelName(model); a->setMaxTokens(4096); a->setTemperature(0.7);
    return a;
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createOllamaAnalyzer(
    const QString &base, const QString &model, QObject *p)
{
    auto *a = new DltLlmAnalyzerInterface(p);
    a->setApiEndpoint(base + "/api/generate");
    a->setApiKey(QString()); a->setModelName(model); a->setMaxTokens(4096); a->setTemperature(0.7);
    return a;
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createLocalAiAnalyzer(
    const QString &base, const QString &model, QObject *p)
{
    auto *a = new DltLlmAnalyzerInterface(p);
    a->setApiEndpoint(base + "/api/generate");
    a->setApiKey(QString()); a->setModelName(model); a->setMaxTokens(4096); a->setTemperature(0.7);
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

} // namespace dltchat
