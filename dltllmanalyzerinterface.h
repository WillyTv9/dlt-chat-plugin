/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef DLLMANALYZERINTERFACE_H
#define DLLMANALYZERINTERFACE_H

#include "dltanalyzerinterface.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QMutex>

class QDltFile;

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

    QString name() const override { return "DLT LLM Analyzer"; }
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

    bool testConnection(QString *errorMessage = nullptr);

signals:
    void apiEndpointChanged(const QString &endpoint);
    void apiKeyChanged(const QString &key);
    void modelNameChanged(const QString &model);
    void maxTokensChanged(int tokens);
    void temperatureChanged(double temp);
    void timeoutChanged(int ms);
    void connectionTestResult(bool success, const QString &message);

private slots:
    void onRequestFinished();
    void onRequestError(QNetworkReply::NetworkError error);

private:
    QString buildPrompt(const QString &query,
                       const QVector<LogEntry> &entries,
                       int maxEntries) const;
    QString parseLlmResponse(const QString &response) const;
    QList<int> extractIndicesFromText(const QString &text) const;

    bool validateConfiguration() const;

    QString m_apiEndpoint;
    QString m_apiKey;
    QString m_modelName;
    int m_maxTokens;
    double m_temperature;
    int m_timeout;

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QByteArray m_pendingResponse;
    bool m_requestInProgress;
    mutable QMutex m_requestMutex;  // Protect m_requestInProgress
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

#endif // DLLMANALYZERINTERFACE_H
