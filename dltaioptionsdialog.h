/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef DLTAIOPTIONSDIALOG_H
#define DLTAIOPTIONSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

class DltAiOptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DltAiOptionsDialog(QWidget *parent = nullptr);

    QString endpoint() const;
    QString apiKey() const;
    QString model() const;
    int maxTokens() const;
    double temperature() const;
    int timeoutMs() const;

    void setEndpoint(const QString &v);
    void setApiKey(const QString &v);
    void setModel(const QString &v);
    void setMaxTokens(int v);
    void setTemperature(double v);
    void setTimeoutMs(int v);

private slots:
    void onProviderChanged(int idx);
    void onTestConnection();

private:
    QComboBox *m_provider;
    QLineEdit *m_endpoint;
    QLineEdit *m_apiKey;
    QLineEdit *m_model;
    QSpinBox *m_maxTokens;
    QDoubleSpinBox *m_temperature;
    QSpinBox *m_timeout;
    QPushButton *m_testBtn;
    QLabel *m_statusLabel;
};

#endif
