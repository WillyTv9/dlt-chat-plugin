/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "dltllmanalyzerinterface.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "dltanalyzerinterface.h"

DltLlmAnalyzerInterface::DltLlmAnalyzerInterface(QObject *parent)
    : QObject(parent)
    , m_maxTokens(1000)
    , m_temperature(0.3)
    , m_timeout(30000)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_requestInProgress(false)
{
    Q_UNUSED(parent);
}

DltLlmAnalyzerInterface::~DltLlmAnalyzerInterface()
{
    if (m_currentReply)
    {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
}

bool DltLlmAnalyzerInterface::isAvailable() const
{
    return validateConfiguration();
}

bool DltLlmAnalyzerInterface::validateConfiguration() const
{
    if (m_apiEndpoint.isEmpty())
    {
        return false;
    }

    bool isLocalEndpoint = m_apiEndpoint.contains("localhost") ||
                           m_apiEndpoint.contains("127.0.0.1") ||
                           m_apiEndpoint.contains("ollama") ||
                           m_apiEndpoint.contains("local-ai");

    if (isLocalEndpoint)
    {
        return !m_modelName.isEmpty();
    }

    return !m_apiKey.isEmpty() && !m_modelName.isEmpty();
}

QString DltLlmAnalyzerInterface::buildPrompt(const QString &query,
                                            const QVector<LogEntry> &entries,
                                            int maxEntries) const
{
    QStringList contextLines;
    const int count = qMin(entries.size(), maxEntries);

    for (int i = 0; i < count; ++i)
    {
        const auto &entry = entries[i];
        contextLines.append(QString("[%1] %2 %3 %4/%5 - %6")
                                .arg(entry.index)
                                .arg(entry.time)
                                .arg(entry.level.toUpper())
                                .arg(entry.apid)
                                .arg(entry.ctid)
                                .arg(entry.payload));
    }

    return QString(
        "Sei un assistente per l'analisi di log DLT (Diagnostic Log and Trace).\n"
        "Rispondi in italiano.\n"
        "Il tuo compito e' rispondere alla domanda dell'utente basandoti sui log forniti.\n\n"
        "LOG (totale %1 entries):\n"
        "%2\n\n"
        "DOMANDA: %3\n\n"
        "RISPOSTA (includi gli indici dei messaggi rilevanti nel formato [index:N]):"
    ).arg(entries.size())
     .arg(contextLines.join("\n"))
     .arg(query);
}

QString DltLlmAnalyzerInterface::parseLlmResponse(const QString &response) const
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError)
    {
        return response.trimmed();
    }

    bool isOpenAI = m_apiEndpoint.contains("openai.com") || m_apiEndpoint.contains("azure");

    if (doc.isObject())
    {
        QJsonObject obj = doc.object();

        if (isOpenAI && obj.contains("choices"))
        {
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty())
            {
                QJsonObject firstChoice = choices[0].toObject();
                if (firstChoice.contains("message"))
                {
                    QJsonObject message = firstChoice["message"].toObject();
                    return message["content"].toString().trimmed();
                }
            }
        }

        if (obj.contains("response"))
        {
            return obj["response"].toString().trimmed();
        }

        if (obj.contains("text"))
        {
            return obj["text"].toString().trimmed();
        }
    }

    return response.trimmed();
}

QList<int> DltLlmAnalyzerInterface::extractIndicesFromText(const QString &text) const
{
    QList<int> indices;
    QRegularExpression re("\\[index:\\s*(\\d+)\\]", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        indices.append(match.captured(1).toInt());
    }

    if (indices.isEmpty())
    {
        QRegularExpression plainRe("\\b(\\d+)\\b");
        QRegularExpressionMatchIterator pit = plainRe.globalMatch(text);
        while (pit.hasNext() && indices.size() < 20)
        {
            QRegularExpressionMatch match = pit.next();
            int num = match.captured(1).toInt();
            if (num < 10000 && !indices.contains(num))
            {
                indices.append(num);
            }
        }
    }

    return indices;
}

DltAnalyzerInterface::QueryResult DltLlmAnalyzerInterface::analyzeQuery(
    const QString &query,
    const QVector<LogEntry> &entries)
{
    QElapsedTimer timer;
    timer.start();

    QueryResult result;

    if (!isAvailable())
    {
        result.responseHtml = "LLM non configurato. Usa l'analyzer rule-based oppure configura un modello LLM.";
        result.success = false;
        result.errorMessage = "LLM analyzer not available: missing endpoint or API key";
        result.processingTimeMs = timer.elapsed();
        return result;
    }

    if (entries.isEmpty())
    {
        result.responseHtml = "Nessun log caricato. Apri un file DLT e riprova.";
        result.success = false;
        result.processingTimeMs = timer.elapsed();
        return result;
    }

    if (query.trimmed().isEmpty())
    {
        result.responseHtml = "Inserisci una domanda o una parola chiave.";
        result.success = false;
        result.processingTimeMs = timer.elapsed();
        return result;
    }

    if (m_requestInProgress)
    {
        QMutexLocker locker(&m_requestMutex);
        if (m_requestInProgress)  // Double-check locking
        {
            result.responseHtml = "Richiesta gia' in corso. Attendi il completamento.";
            result.success = false;
            result.processingTimeMs = timer.elapsed();
            return result;
        }
    }

    QUrl url(m_apiEndpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty())
    {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    request.setTransferTimeout(m_timeout);
#endif

    const int maxEntries = qMin(entries.size(), 500);
    QString prompt = buildPrompt(query, entries, maxEntries);

    QJsonObject json;
    json["model"] = m_modelName;
    json["stream"] = false;

    bool isOpenAI = m_apiEndpoint.contains("openai.com") || m_apiEndpoint.contains("azure");

    if (isOpenAI)
    {
        QJsonArray messages;
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = "Sei un assistente per l'analisi di log DLT (Diagnostic Log and Trace). Rispondi in italiano.";
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = prompt;
        messages.append(systemMsg);
        messages.append(userMsg);
        json["messages"] = messages;

        if (m_maxTokens > 0)
        {
            json["max_tokens"] = m_maxTokens;
            json["temperature"] = m_temperature;
        }
    }
    else
    {
        json["prompt"] = prompt;
        if (m_maxTokens > 0)
        {
            json["options"] = QJsonObject{
                {"num_predict", m_maxTokens},
                {"temperature", m_temperature}
            };
        }
    }

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    m_requestInProgress = true;
    m_pendingResponse.clear();

    m_currentReply = m_networkManager->post(request, data);

    connect(m_currentReply, &QNetworkReply::finished, this, &DltLlmAnalyzerInterface::onRequestFinished);
    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &DltLlmAnalyzerInterface::onRequestError);

    // Use QEventLoop with timeout instead of busy-wait
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(this, &DltLlmAnalyzerInterface::onRequestFinished, &loop, &QEventLoop::quit);
    
    timeoutTimer.start(m_timeout);
    loop.exec();
    
    if (!timeoutTimer.isActive())
    {
        // Timeout occurred
        if (m_currentReply)
        {
            m_currentReply->abort();
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
        }
        result.responseHtml = "Timeout nella risposta del LLM. Prova con un numero minore di log entries.";
        result.success = false;
        result.errorMessage = "LLM request timeout";
        result.processingTimeMs = timer.elapsed();
        m_requestInProgress = false;
        return result;
    }
    timeoutTimer.stop();

    if (m_pendingResponse.isEmpty())
    {
        result.responseHtml = "Il modello LLM non ha risposto. Controlla la configurazione.";
        result.success = false;
        result.errorMessage = "Empty LLM response";
        result.processingTimeMs = timer.elapsed();
        return result;
    }

    QString responseText = parseLlmResponse(QString::fromUtf8(m_pendingResponse));
    result.responseHtml = responseText;

    result.indices = extractIndicesFromText(responseText);
    result.snippets.reserve(result.indices.size());
    QSet<int> seen;
    for (int idx : result.indices)
    {
        if (seen.contains(idx))
        {
            continue;
        }
        seen.insert(idx);
        for (const auto &entry : entries)
        {
            if (entry.index == idx)
            {
                result.snippets.append(entry.payload.left(120));
                break;
            }
        }
    }

    result.success = true;
    result.processingTimeMs = timer.elapsed();
    return result;
}

void DltLlmAnalyzerInterface::onRequestFinished()
{
    m_requestInProgress = false;

    if (!m_currentReply)
    {
        return;
    }

    if (m_currentReply->error() == QNetworkReply::NoError)
    {
        m_pendingResponse = m_currentReply->readAll();
    }
    else
    {
        qWarning() << "LLM Network Error:" << m_currentReply->errorString();
    }

    // Ensure proper cleanup
    disconnect(m_currentReply, nullptr, this, nullptr);
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void DltLlmAnalyzerInterface::onRequestError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    m_requestInProgress = false;

    if (m_currentReply)
    {
        m_pendingResponse = m_currentReply->readAll();
    }
}

QString DltLlmAnalyzerInterface::configurationInfo() const
{
    if (!isAvailable())
    {
        return "LLM Analyzer non configurato.\n"
               "Imposta API endpoint, API key e model name.";
    }

    return QString("LLM Analyzer\n"
                  "- Provider: OpenAI Compatible\n"
                  "- Endpoint: %1\n"
                  "- Model: %2\n"
                  "- Max Tokens: %3\n"
                  "- Temperature: %4\n"
                  "- Timeout: %5ms")
        .arg(m_apiEndpoint)
        .arg(m_modelName)
        .arg(m_maxTokens)
        .arg(m_temperature)
        .arg(m_timeout);
}

QStringList DltLlmAnalyzerInterface::supportedLanguages() const
{
    return QStringList{"en", "it", "de", "es", "fr", "zh", "ja"};
}

bool DltLlmAnalyzerInterface::configure(const QVariantMap &config)
{
    if (config.contains("apiEndpoint"))
    {
        setApiEndpoint(config["apiEndpoint"].toString());
    }
    if (config.contains("apiKey"))
    {
        setApiKey(config["apiKey"].toString());
    }
    if (config.contains("modelName"))
    {
        setModelName(config["modelName"].toString());
    }
    if (config.contains("maxTokens"))
    {
        setMaxTokens(config["maxTokens"].toInt());
    }
    if (config.contains("temperature"))
    {
        setTemperature(config["temperature"].toDouble());
    }
    if (config.contains("timeout"))
    {
        setTimeout(config["timeout"].toInt());
    }
    return true;
}

QVariantMap DltLlmAnalyzerInterface::currentConfiguration() const
{
    QVariantMap config;
    config["type"] = "llm";
    config["apiEndpoint"] = m_apiEndpoint;
    config["modelName"] = m_modelName;
    config["maxTokens"] = m_maxTokens;
    config["temperature"] = m_temperature;
    config["timeout"] = m_timeout;
    config["hasApiKey"] = !m_apiKey.isEmpty();
    return config;
}

bool DltLlmAnalyzerInterface::testConnection(QString *errorMessage)
{
    if (m_apiEndpoint.isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = "API endpoint non impostato";
        }
        emit connectionTestResult(false, "API endpoint non impostato");
        return false;
    }

    QUrl url(m_apiEndpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty())
    {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    }

    QJsonObject json;
    json["model"] = m_modelName.isEmpty() ? "test" : m_modelName;
    json["prompt"] = "test";
    json["stream"] = false;

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());

    bool success = false;
    QString message;

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(10000);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();

    if (timer.isActive())
    {
        success = (reply->error() == QNetworkReply::NoError);
        if (!success)
        {
            message = reply->errorString();
        }
        else
        {
            message = "Connessione riuscita";
        }
    }
    else
    {
        message = "Timeout connessione";
    }

    reply->deleteLater();

    if (errorMessage)
    {
        *errorMessage = message;
    }
    emit connectionTestResult(success, message);
    return success;
}

void DltLlmAnalyzerInterface::setApiEndpoint(const QString &endpoint)
{
    if (m_apiEndpoint != endpoint)
    {
        m_apiEndpoint = endpoint;
        emit apiEndpointChanged(endpoint);
    }
}

void DltLlmAnalyzerInterface::setApiKey(const QString &key)
{
    if (m_apiKey != key)
    {
        m_apiKey = key;
        emit apiKeyChanged(key);
    }
}

void DltLlmAnalyzerInterface::setModelName(const QString &model)
{
    if (m_modelName != model)
    {
        m_modelName = model;
        emit modelNameChanged(model);
    }
}

void DltLlmAnalyzerInterface::setMaxTokens(int tokens)
{
    if (m_maxTokens != tokens)
    {
        m_maxTokens = tokens;
        emit maxTokensChanged(tokens);
    }
}

void DltLlmAnalyzerInterface::setTemperature(double temp)
{
    if (qAbs(m_temperature - temp) > 0.001)
    {
        m_temperature = temp;
        emit temperatureChanged(temp);
    }
}

void DltLlmAnalyzerInterface::setTimeout(int ms)
{
    if (m_timeout != ms)
    {
        m_timeout = ms;
        emit timeoutChanged(ms);
    }
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createOpenAIAnalyzer(
    const QString &apiKey,
    const QString &model,
    QObject *parent)
{
    DltLlmAnalyzerInterface *analyzer = new DltLlmAnalyzerInterface(parent);
    analyzer->setApiEndpoint("https://api.openai.com/v1/completions");
    analyzer->setApiKey(apiKey);
    analyzer->setModelName(model);
    analyzer->setMaxTokens(1000);
    analyzer->setTemperature(0.3);
    return analyzer;
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createOllamaAnalyzer(
    const QString &baseUrl,
    const QString &model,
    QObject *parent)
{
    DltLlmAnalyzerInterface *analyzer = new DltLlmAnalyzerInterface(parent);
    analyzer->setApiEndpoint(baseUrl + "/api/generate");
    analyzer->setApiKey(QString());
    analyzer->setModelName(model);
    analyzer->setMaxTokens(1000);
    analyzer->setTemperature(0.3);
    return analyzer;
}

DltLlmAnalyzerInterface *DltLlmAnalyzerFactory::createLocalAiAnalyzer(
    const QString &baseUrl,
    const QString &model,
    QObject *parent)
{
    DltLlmAnalyzerInterface *analyzer = new DltLlmAnalyzerInterface(parent);
    analyzer->setApiEndpoint(baseUrl + "/api/generate");
    analyzer->setApiKey(QString());
    analyzer->setModelName(model);
    analyzer->setMaxTokens(1000);
    analyzer->setTemperature(0.3);
    return analyzer;
}

QStringList DltLlmAnalyzerFactory::availableProviders()
{
    return QStringList{"openai", "ollama", "local-ai"};
}

QString DltLlmAnalyzerFactory::defaultModelForProvider(const QString &provider)
{
    if (provider == "openai")
    {
        return "gpt-4";
    }
    if (provider == "ollama")
    {
        return "llama3";
    }
    if (provider == "local-ai")
    {
        return "mistral";
    }
    return "llama3";
}
