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
#include <QGroupBox>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QApplication>

static const QString kCopilotClientId = "Iv1.b507a08c87ecfe98";
static const QString kCopilotEndpoint = "https://api.githubcopilot.com/chat/completions";

DltAiOptionsDialog::DltAiOptionsDialog(QWidget *parent)
    : QDialog(parent)
    , m_pollTimer(new QTimer(this))
{
    setWindowTitle(tr("AI Configuration"));
    setMinimumWidth(480);

    // Provider combo: Local providers first, then cloud
    m_provider = new QComboBox(this);
    m_provider->addItem("Ollama (local)",    "http://localhost:11434/api/generate");
    m_provider->addItem("LM Studio (local)", "http://localhost:1234/v1/chat/completions");
    m_provider->addItem("LocalAI (local)",   "http://localhost:8080/v1/chat/completions");
    m_provider->insertSeparator(3);
    m_provider->addItem("OpenAI",            "https://api.openai.com/v1/chat/completions");
    m_provider->addItem("DeepSeek",          "https://api.deepseek.com/v1/chat/completions");
    m_provider->addItem("Claude (Anthropic)","https://api.anthropic.com/v1/messages");
    m_provider->addItem("GitHub Copilot",    kCopilotEndpoint);
    m_provider->addItem(tr("Custom"),        "");

    m_endpoint = new QLineEdit("http://localhost:11434/api/generate", this);
    m_apiKey = new QLineEdit(this);
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setPlaceholderText(tr("Not required for Ollama / LM Studio"));
    m_model = new QLineEdit("llama3.2:1b", this);
    m_modelHint = new QLabel(tr("Ollama: llama3.2:1b · OpenAI: gpt-4o · Copilot: gpt-4o"), this);
    m_modelHint->setStyleSheet("font-size: 9px; color: #888; font-style: italic;");
    m_modelHint->setWordWrap(true);
    m_maxTokens = new QSpinBox(this);
    m_maxTokens->setRange(64, 8192);
    m_maxTokens->setValue(4096);
    m_temperature = new QDoubleSpinBox(this);
    m_temperature->setRange(0.0, 2.0);
    m_temperature->setSingleStep(0.1);
    m_temperature->setValue(0.7);
    m_timeout = new QSpinBox(this);
    m_timeout->setRange(5000, 120000);
    m_timeout->setSingleStep(5000);
    m_timeout->setValue(120000);
    m_timeout->setSuffix(" ms");

    m_testBtn = new QPushButton(tr("Test Connection"), this);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-size: 11px;");

    // Copilot authentication panel
    m_copilotPanel = new QWidget(this);
    m_copilotPanel->setVisible(false);

    m_copilotStatusLabel = new QLabel(tr("Not authenticated"), m_copilotPanel);
    m_copilotStatusLabel->setStyleSheet("font-size: 11px; color: #888;");
    m_copilotStatusLabel->setWordWrap(true);

    m_signInBtn = new QPushButton(tr("Sign in with GitHub"), m_copilotPanel);
    m_useExistingBtn = new QPushButton(tr("Auto-detect token"), m_copilotPanel);

    m_deviceCodeLabel = new QLabel(m_copilotPanel);
    m_deviceCodeLabel->setVisible(false);
    m_deviceCodeLabel->setWordWrap(true);
    m_deviceCodeLabel->setStyleSheet("font-size: 10px; color: #555;");

    m_manualTokenEdit = new QLineEdit(m_copilotPanel);
    m_manualTokenEdit->setPlaceholderText(tr("Paste gho_* token manually (optional)"));
    m_manualTokenEdit->setEchoMode(QLineEdit::Password);

    QVBoxLayout *copilotLayout = new QVBoxLayout(m_copilotPanel);
    copilotLayout->setContentsMargins(0, 4, 0, 0);
    QHBoxLayout *copilotBtnRow = new QHBoxLayout;
    copilotBtnRow->addWidget(m_useExistingBtn);
    copilotBtnRow->addWidget(m_signInBtn);
    copilotBtnRow->addStretch();
    copilotLayout->addLayout(copilotBtnRow);
    copilotLayout->addWidget(m_deviceCodeLabel);
    copilotLayout->addWidget(m_copilotStatusLabel);
    copilotLayout->addWidget(new QLabel(tr("Manual token:"), m_copilotPanel));
    copilotLayout->addWidget(m_manualTokenEdit);

    // Main form
    QFormLayout *form = new QFormLayout();
    form->addRow(tr("Provider:"), m_provider);
    form->addRow(tr("Endpoint:"), m_endpoint);
    form->addRow(tr("API Key:"), m_apiKey);
    form->addRow(tr("Model:"), m_model);
    form->addRow("", m_modelHint);
    form->addRow(tr("Max Tokens:"), m_maxTokens);
    form->addRow(tr("Temperature:"), m_temperature);
    form->addRow(tr("Timeout:"), m_timeout);
    form->addRow(tr("Copilot Auth:"), m_copilotPanel);

    QHBoxLayout *testLayout = new QHBoxLayout();
    testLayout->addWidget(m_testBtn);
    testLayout->addWidget(m_statusLabel, 1);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addLayout(testLayout);
    mainLayout->addWidget(buttons);

    m_pollTimer->setInterval(5000);

    connect(m_provider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DltAiOptionsDialog::onProviderChanged);
    connect(m_testBtn,       &QPushButton::clicked, this, &DltAiOptionsDialog::onTestConnection);
    connect(m_signInBtn,     &QPushButton::clicked, this, &DltAiOptionsDialog::onSignInWithGitHub);
    connect(m_useExistingBtn,&QPushButton::clicked, this, &DltAiOptionsDialog::onUseExistingToken);
    connect(m_pollTimer,     &QTimer::timeout,       this, &DltAiOptionsDialog::pollOAuthToken);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Try auto-detecting a Copilot token at startup (silent)
    QString tok = detectCopilotTokenFromFilesystem();
    if (!tok.isEmpty()) {
        m_copilotOAuthToken = tok;
        setCopilotStatus(tr("Token found automatically"), true);
    }
}

// ---- provider change ----

void DltAiOptionsDialog::onProviderChanged(int idx)
{
    updateEndpointForProvider(idx);

    // Show separator items as non-selectable
    if (m_provider->itemData(idx).toString().isEmpty() &&
        m_provider->itemText(idx).isEmpty()) {
        m_provider->setCurrentIndex(idx > 0 ? idx - 1 : idx + 1);
        return;
    }

    bool isCopilot = (m_provider->itemText(idx) == "GitHub Copilot");
    m_copilotPanel->setVisible(isCopilot);

    if (isCopilot) {
        m_model->setText("gpt-4o");
        m_apiKey->setPlaceholderText(tr("Leave empty — Copilot token used automatically"));
    }
}

void DltAiOptionsDialog::updateEndpointForProvider(int idx)
{
    const QString endpointData = m_provider->itemData(idx).toString();
    if (!endpointData.isEmpty())
        m_endpoint->setText(endpointData);
}

// ---- Copilot auto-detect ----

QString DltAiOptionsDialog::detectCopilotTokenFromFilesystem()
{
    QStringList candidates;
#ifdef Q_OS_WIN
    QString localApp = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    candidates << localApp + "/GitHub Copilot/apps.json";
    // Also check LOCALAPPDATA directly
    QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (!localAppData.isEmpty())
        candidates << localAppData + "/GitHub Copilot/apps.json";
#else
    QString home = QDir::homePath();
    candidates << home + "/.config/github-copilot/apps.json";
    candidates << home + "/.config/github-copilot/hosts.json";
#endif

    for (const QString &path : candidates) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) continue;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (!doc.isObject()) continue;
        QJsonObject root = doc.object();
        // apps.json: { "user": { "oauth_token": "gho_*" } }
        if (root.contains("user")) {
            QString tok = root["user"].toObject()["oauth_token"].toString();
            if (tok.startsWith("gho_")) return tok;
        }
        // hosts.json: { "github.com": { "oauth_token": "gho_*" } }
        for (auto it = root.begin(); it != root.end(); ++it) {
            QString tok = it.value().toObject()["oauth_token"].toString();
            if (tok.startsWith("gho_")) return tok;
        }
    }
    return {};
}

void DltAiOptionsDialog::onUseExistingToken()
{
    QString tok = detectCopilotTokenFromFilesystem();
    if (tok.isEmpty()) {
        // Try manual field
        tok = m_manualTokenEdit->text().trimmed();
    }
    if (tok.isEmpty()) {
        setCopilotStatus(tr("No token found. Try signing in or paste manually."), false);
        return;
    }
    m_copilotOAuthToken = tok;
    m_apiKey->setText(tok);
    setCopilotStatus(tr("Token loaded — test connection to verify"), true);
}

// ---- OAuth Device Flow ----

void DltAiOptionsDialog::onSignInWithGitHub()
{
    m_signInBtn->setEnabled(false);
    m_deviceCodeLabel->setVisible(false);
    setCopilotStatus(tr("Requesting device code…"), false);
    QCoreApplication::processEvents();

    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl("https://github.com/login/device/code"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Accept", "application/json");

    QByteArray body = QString("client_id=%1&scope=copilot").arg(kCopilotClientId).toUtf8();
    QNetworkReply *reply = mgr.post(req, body);
    QEventLoop loop;
    QTimer t; t.setSingleShot(true); t.setInterval(10000);
    connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    t.start(); loop.exec();

    if (!t.isActive() || reply->error() != QNetworkReply::NoError) {
        setCopilotStatus(tr("Failed to reach GitHub: ") + reply->errorString(), false);
        m_signInBtn->setEnabled(true);
        reply->deleteLater();
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    reply->deleteLater();

    QString userCode   = obj["user_code"].toString();
    QString verifyUri  = obj["verification_uri"].toString();
    m_deviceCode       = obj["device_code"].toString();
    int interval       = qMax(5, obj["interval"].toInt(5));

    m_pollTimer->setInterval(interval * 1000);

    m_deviceCodeLabel->setText(
        tr("1. Open: %1\n2. Enter code: <b>%2</b>\n3. Waiting for authorization…")
        .arg(verifyUri, userCode));
    m_deviceCodeLabel->setVisible(true);

    QApplication::clipboard()->setText(userCode);
    QDesktopServices::openUrl(QUrl(verifyUri));

    setCopilotStatus(tr("Code copied to clipboard. Authorize in the browser."), false);
    m_pollTimer->start();
    m_signInBtn->setEnabled(true);
}

void DltAiOptionsDialog::pollOAuthToken()
{
    if (m_deviceCode.isEmpty()) { m_pollTimer->stop(); return; }

    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl("https://github.com/login/oauth/access_token"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Accept", "application/json");

    QByteArray body = QString(
        "client_id=%1&device_code=%2&grant_type=urn:ietf:params:oauth:grant-type:device_code")
        .arg(kCopilotClientId, m_deviceCode).toUtf8();

    QNetworkReply *reply = mgr.post(req, body);
    QEventLoop loop;
    QTimer t; t.setSingleShot(true); t.setInterval(8000);
    connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    t.start(); loop.exec();

    if (!t.isActive()) { reply->deleteLater(); return; }

    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    reply->deleteLater();

    QString accessToken = obj["access_token"].toString();
    QString error       = obj["error"].toString();

    if (!accessToken.isEmpty()) {
        m_pollTimer->stop();
        m_deviceCode.clear();
        m_deviceCodeLabel->setVisible(false);
        m_copilotOAuthToken = accessToken;
        m_apiKey->setText(accessToken);
        setCopilotStatus(tr("Authenticated with GitHub Copilot"), true);
    } else if (error == "authorization_pending" || error == "slow_down") {
        // still waiting — keep polling
    } else {
        m_pollTimer->stop();
        m_deviceCode.clear();
        setCopilotStatus(tr("Authorization failed: ") + error, false);
    }
}

// ---- Test Connection ----

void DltAiOptionsDialog::onTestConnection()
{
    m_statusLabel->setText(tr("Testing…"));
    m_statusLabel->setStyleSheet("color: #999; font-size: 11px;");
    m_testBtn->setEnabled(false);
    QCoreApplication::processEvents();

    QNetworkAccessManager mgr;
    QUrl url(m_endpoint->text());
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString effectiveKey = m_apiKey->text().isEmpty() ? m_copilotOAuthToken : m_apiKey->text();
    if (!effectiveKey.isEmpty())
        req.setRawHeader("Authorization", QString("Bearer %1").arg(effectiveKey).toUtf8());

    bool isCopilot = m_provider->currentText() == "GitHub Copilot";
    if (isCopilot) {
        req.setRawHeader("Editor-Version", "DLTChatPlugin/1.0");
        req.setRawHeader("Copilot-Integration-Id", "dlt-chat-plugin");
    }

    QJsonObject body;
    body["model"] = m_model->text().isEmpty() ? "test" : m_model->text();
    body["stream"] = false;

    bool isOpenAIStyle = (m_provider->currentIndex() != 0); // not Ollama old-style
    if (isOpenAIStyle) {
        QJsonArray msgs;
        QJsonObject u; u["role"] = "user"; u["content"] = "test";
        msgs.append(u);
        body["messages"] = msgs;
        body["max_tokens"] = 1;
    } else {
        body["prompt"] = "test";
        body["options"] = QJsonObject{{"num_predict", 1}};
    }

    QNetworkReply *reply = mgr.post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(m_timeout->value());
    loop.exec();

    m_testBtn->setEnabled(true);

    if (!timer.isActive()) {
        reply->abort();
        m_statusLabel->setText(tr("✖ Timeout"));
        m_statusLabel->setStyleSheet("color: #c62828; font-size: 11px;");
    } else {
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // 200 = OK, 401/403 = server reachable but auth wrong, 400 = bad request (model ok)
        bool reachable = (httpCode > 0 || reply->error() == QNetworkReply::NoError);
        if (reachable) {
            m_statusLabel->setText(tr("✔ Connection OK (HTTP %1)").arg(httpCode));
            m_statusLabel->setStyleSheet("color: #2e7d32; font-size: 11px;");
        } else {
            m_statusLabel->setText(QString("✖ %1").arg(reply->errorString()));
            m_statusLabel->setStyleSheet("color: #c62828; font-size: 11px;");
        }
    }
    reply->deleteLater();
}

// ---- Copilot status helper ----

void DltAiOptionsDialog::setCopilotStatus(const QString &text, bool ok)
{
    m_copilotStatusLabel->setText(text);
    m_copilotStatusLabel->setStyleSheet(
        ok ? "font-size: 11px; color: #2e7d32;"
           : "font-size: 11px; color: #888;");
}

// ---- Getters / setters ----

QString DltAiOptionsDialog::endpoint()           const { return m_endpoint->text(); }
QString DltAiOptionsDialog::apiKey()             const { return m_apiKey->text(); }
QString DltAiOptionsDialog::model()              const { return m_model->text(); }
int     DltAiOptionsDialog::maxTokens()          const { return m_maxTokens->value(); }
double  DltAiOptionsDialog::temperature()        const { return m_temperature->value(); }
int     DltAiOptionsDialog::timeoutMs()          const { return m_timeout->value(); }
QString DltAiOptionsDialog::copilotOAuthToken()  const { return m_copilotOAuthToken; }

void DltAiOptionsDialog::setEndpoint(const QString &v)  { m_endpoint->setText(v); }
void DltAiOptionsDialog::setApiKey(const QString &v)    { m_apiKey->setText(v); }
void DltAiOptionsDialog::setModel(const QString &v)     { m_model->setText(v); }
void DltAiOptionsDialog::setMaxTokens(int v)            { m_maxTokens->setValue(v); }
void DltAiOptionsDialog::setTemperature(double v)       { m_temperature->setValue(v); }
void DltAiOptionsDialog::setTimeoutMs(int v)            { m_timeout->setValue(v); }
void DltAiOptionsDialog::setCopilotOAuthToken(const QString &token)
{
    m_copilotOAuthToken = token;
    if (!token.isEmpty())
        setCopilotStatus(tr("Token configured"), true);
}
