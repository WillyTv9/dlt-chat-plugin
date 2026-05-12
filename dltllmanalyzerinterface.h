#ifndef DLLMANALYZERINTERFACE_H
#define DLLMANALYZERINTERFACE_H

#include "dltanalyzerinterface.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QSet>
#include <QElapsedTimer>
#include <QQueue>
#include <QHash>
#include <QDateTime>

#define DLT_LLM_INTERFACE_VERSION "1.0.0"

class DltLlmAnalyzerInterface : public QObject, public DltAnalyzerInterface
{
    Q_OBJECT
public:
    Q_INTERFACES(DltAnalyzerInterface)
    Q_PROPERTY(QString apiEndpoint READ apiEndpoint WRITE setApiEndpoint NOTIFY apiEndpointChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(int maxTokens READ maxTokens WRITE setMaxTokens NOTIFY maxTokensChanged)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged)

public:
    explicit DltLlmAnalyzerInterface(QObject *parent = nullptr);
    ~DltLlmAnalyzerInterface() override;

    QString name() const override { return "LLM Analyzer"; }
    QString version() const override { return DLT_LLM_INTERFACE_VERSION; }
    QString interfaceVersion() const override { return DLT_ANALYZER_INTERFACE_VERSION; }

    bool isAvailable() const override;
    bool supportsStreaming() const override { return false; }

    QueryResult analyzeQuery(const QString &query,
                             const QVector<LogEntry> &entries) override;

    QString configurationInfo() const override;
    QStringList supportedLanguages() const override;

    bool configure(const QVariantMap &config) override;
    QVariantMap currentConfiguration() const override;

    QString apiEndpoint() const { return m_apiEndpoint; }
    QString apiKey() const { return m_apiKey; }
    QString modelName() const { return m_modelName; }
    int maxTokens() const { return m_maxTokens; }
    double temperature() const { return m_temperature; }
    int timeout() const { return m_timeout; }

    void setApiEndpoint(const QString &endpoint);
    void setApiKey(const QString &key);
    void setModelName(const QString &model);
    void setMaxTokens(int tokens);
    void setTemperature(double temp);
    void setTimeout(int ms);

    bool validateConfiguration() const;
    bool testConnection(QString *errorMessage = nullptr);

    bool analyzeQueryAsync(const QString &query, const QVector<LogEntry> &entries);
    void setExtraContext(const QString &context) { m_extraContext = context; }

    // Utility methods exposed for testing
    QString parseLlmResponse(const QString &response) const;
    QList<int> extractIndicesFromText(const QString &text) const;
    QString buildPrompt(const QString &query,
                        const QVector<LogEntry> &entries,
                        int maxEntries) const;
    QByteArray buildRequestBody(const QString &prompt) const;

signals:
    void apiEndpointChanged(const QString &endpoint);
    void apiKeyChanged(const QString &key);
    void modelNameChanged(const QString &model);
    void maxTokensChanged(int tokens);
    void temperatureChanged(double temp);
    void timeoutChanged(int ms);
    void connectionTestResult(bool success, const QString &message);
    void queryResultReady(const DltAnalyzerInterface::QueryResult &result, const QString &originalQuery);

private:
    QueryResult processReply(QNetworkReply *reply, const QElapsedTimer &timer);

    // Rate limiting: token bucket
    struct TokenBucket {
        qint64 lastRefill = 0;
        double tokens = 0;
        double capacity = 10; // max tokens
        double refillRate = 1.0; // tokens per second
        QMutex mutex;
    };
    TokenBucket m_rateLimiter;

    // Circuit breaker
    enum CircuitState { Closed, Open, HalfOpen };
    CircuitState m_circuitState = Closed;
    qint64 m_lastFailureTime = 0;
    static constexpr int CIRCUIT_OPEN_TIMEOUT_MS = 60000; // 1 minute
    static constexpr int CIRCUIT_FAILURE_THRESHOLD = 5;
    int m_failureCount = 0;
    QMutex m_circuitMutex;

    // LRU Cache for responses
    struct CacheEntry {
        QueryResult result;
        QDateTime timestamp;
    };
    QHash<QString, CacheEntry> m_responseCache;
    static constexpr int CACHE_MAX_SIZE = 1000;
    QMutex m_cacheMutex;

    QString m_apiEndpoint;
    QString m_apiKey;
    QString m_modelName;
    int m_maxTokens;
    double m_temperature;
    int m_timeout;

    QNetworkAccessManager *m_networkManager;
    mutable QMutex m_availMutex;
    mutable bool m_availabilityVerified = false;
    mutable qint64 m_lastAvailabilityCheck = 0;
    static constexpr int AVAILABILITY_TTL_MS = 30000;
    QString m_extraContext;
};

class DltLlmAnalyzerFactory : public QObject
{
    Q_OBJECT
public:
    static DltLlmAnalyzerInterface *createOpenAIAnalyzer(const QString &apiKey,
                                                         const QString &model = "gpt-4",
                                                         QObject *parent = nullptr);
    static DltLlmAnalyzerInterface *createOllamaAnalyzer(const QString &baseUrl = "http://localhost:11434",
                                                         const QString &model = "llama3",
                                                         QObject *parent = nullptr);
    static DltLlmAnalyzerInterface *createLocalAiAnalyzer(const QString &baseUrl,
                                                          const QString &model,
                                                          QObject *parent = nullptr);
    static QStringList availableProviders();
    static QString defaultModelForProvider(const QString &provider);
};

#endif