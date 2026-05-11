/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "dltaioptionsdialog.h"

#include <QCoreApplication>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>

DltAiOptionsDialog::DltAiOptionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("AI Configuration"));                   // Italian: Configurazione AI
    setMinimumWidth(450);

    m_provider = new QComboBox(this);
    m_provider->addItem("Ollama",        "http://localhost:11434");
    m_provider->addItem("OpenAI",        "https://api.openai.com/v1/chat/completions");
    m_provider->addItem("LocalAI",       "http://localhost:8080");
    m_provider->addItem(tr("Custom"),    "");                  // Italian: Personalizzato

    m_endpoint = new QLineEdit("http://localhost:11434/api/generate", this);
    m_apiKey = new QLineEdit(this);
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setPlaceholderText(tr("Not required (Ollama/LocalAI)")); // Italian: Non richiesta (Ollama/LocalAI)
    m_model = new QLineEdit("qwen3.5:4b", this);
    m_maxTokens = new QSpinBox(this);
    m_maxTokens->setRange(64, 8192);
    m_maxTokens->setValue(1000);
    m_temperature = new QDoubleSpinBox(this);
    m_temperature->setRange(0.0, 2.0);
    m_temperature->setSingleStep(0.1);
    m_temperature->setValue(0.3);
    m_timeout = new QSpinBox(this);
    m_timeout->setRange(5000, 120000);
    m_timeout->setSingleStep(5000);
    m_timeout->setValue(30000);
    m_timeout->setSuffix(" ms");

    m_testBtn = new QPushButton(tr("Test Connection"), this);  // Italian: Test Connessione
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-size: 11px;");

    QFormLayout *form = new QFormLayout();
    form->addRow(tr("Provider:"), m_provider);
    form->addRow(tr("Endpoint:"), m_endpoint);
    form->addRow(tr("API Key:"), m_apiKey);
    form->addRow(tr("Model:"), m_model);                      // Italian: Modello
    form->addRow(tr("Max Tokens:"), m_maxTokens);
    form->addRow(tr("Temperature:"), m_temperature);          // Italian: Temperatura
    form->addRow(tr("Timeout:"), m_timeout);

    QHBoxLayout *testLayout = new QHBoxLayout();
    testLayout->addWidget(m_testBtn);
    testLayout->addWidget(m_statusLabel, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addLayout(testLayout);
    mainLayout->addWidget(buttons);

    connect(m_provider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DltAiOptionsDialog::onProviderChanged);
    connect(m_testBtn, &QPushButton::clicked, this, &DltAiOptionsDialog::onTestConnection);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void DltAiOptionsDialog::onProviderChanged(int idx)
{
    QString base = m_provider->itemData(idx).toString();
    if (idx == 0)
        m_endpoint->setText(base + "/api/generate");
    else if (idx == 1)
        m_endpoint->setText(base);
    else if (idx == 2)
        m_endpoint->setText(base + "/api/generate");
}

void DltAiOptionsDialog::onTestConnection()
{
    m_statusLabel->setText(tr("Testing..."));                  // Italian: Test in corso...
    m_statusLabel->setStyleSheet("color: #999; font-size: 11px;");
    m_testBtn->setEnabled(false);
    QCoreApplication::processEvents();

    QNetworkAccessManager mgr;
    QUrl url(m_endpoint->text());
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_apiKey->text().isEmpty())
        req.setRawHeader("Authorization",
            QString("Bearer %1").arg(m_apiKey->text()).toUtf8());

    QJsonObject body;
    body["model"] = m_model->text().isEmpty() ? "test" : m_model->text();
    body["prompt"] = "test";
    body["stream"] = false;

    QNetworkReply *reply = mgr.post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(m_timeout->value());
    loop.exec();

    m_testBtn->setEnabled(true);

    if (!timer.isActive())
    {
        reply->abort();
        m_statusLabel->setText(tr("\u2716 Timeout"));
        m_statusLabel->setStyleSheet("color: #c62828; font-size: 11px;");
    }
    else if (reply->error() != QNetworkReply::NoError)
    {
        m_statusLabel->setText(QString("\u2716 %1").arg(reply->errorString()));
        m_statusLabel->setStyleSheet("color: #c62828; font-size: 11px;");
    }
    else
    {
        m_statusLabel->setText(tr("\u2714 Connection OK"));    // Italian: Connessione riuscita
        m_statusLabel->setStyleSheet("color: #2e7d32; font-size: 11px;");
    }
    reply->deleteLater();
}

QString DltAiOptionsDialog::endpoint() const { return m_endpoint->text(); }
QString DltAiOptionsDialog::apiKey() const { return m_apiKey->text(); }
QString DltAiOptionsDialog::model() const { return m_model->text(); }
int DltAiOptionsDialog::maxTokens() const { return m_maxTokens->value(); }
double DltAiOptionsDialog::temperature() const { return m_temperature->value(); }
int DltAiOptionsDialog::timeoutMs() const { return m_timeout->value(); }

void DltAiOptionsDialog::setEndpoint(const QString &v) { m_endpoint->setText(v); }
void DltAiOptionsDialog::setApiKey(const QString &v) { m_apiKey->setText(v); }
void DltAiOptionsDialog::setModel(const QString &v) { m_model->setText(v); }
void DltAiOptionsDialog::setMaxTokens(int v) { m_maxTokens->setValue(v); }
void DltAiOptionsDialog::setTemperature(double v) { m_temperature->setValue(v); }
void DltAiOptionsDialog::setTimeoutMs(int v) { m_timeout->setValue(v); }
