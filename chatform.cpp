/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "chatform.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QGroupBox>

namespace DltChat {

Form::Form(QWidget *parent)
    : QWidget(parent)
    , statusLabel(new QLabel(this))
    , history(new QTextBrowser(this))
    , resultsList(new QListWidget(this))
    , input(new QLineEdit(this))
    , sendButton(new QPushButton(tr("Invia"), this))
    , clearButton(new QPushButton(tr("Pulisci"), this))
    , exportCsvButton(new QPushButton(tr("Esporta CSV"), this))
    , exportAllButton(new QPushButton(tr("Esporta Tutto"), this))
    , btnErrors(new QPushButton(tr("Errori"), this))
    , btnWarnings(new QPushButton(tr("Warnings"), this))
    , btnCan(new QPushButton(tr("CAN"), this))
    , btnTimeout(new QPushButton(tr("Timeout"), this))
    , btnPattern(new QPushButton(tr("Pattern"), this))
    , btnTimeline(new QPushButton(tr("Timeline"), this))
    , btnSummary(new QPushButton(tr("Summary"), this))
    , btnHelp(new QPushButton(tr("Help"), this))
    , lastQuery(QString())
{
    QLabel *title = new QLabel(tr("DLT Log Assistant"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);

    statusLabel->setText(tr("Nessun log caricato."));
    statusLabel->setWordWrap(true);

    history->setReadOnly(true);
    history->setOpenExternalLinks(false);

    resultsList->setSelectionMode(QAbstractItemView::SingleSelection);

    input->setPlaceholderText(tr("Fai una domanda sui log..."));

    QLabel *resultsLabel = new QLabel(tr("Risultati"), this);

    QGroupBox *quickActions = new QGroupBox(tr("Azioni Rapide"), this);
    QHBoxLayout *quickLayout = new QHBoxLayout();
    quickLayout->setSpacing(2);
    quickLayout->addWidget(btnErrors);
    quickLayout->addWidget(btnWarnings);
    quickLayout->addWidget(btnCan);
    quickLayout->addWidget(btnTimeout);
    quickLayout->addWidget(btnPattern);
    quickLayout->addWidget(btnTimeline);
    quickLayout->addWidget(btnSummary);
    quickLayout->addWidget(btnHelp);
    quickLayout->addStretch();
    quickActions->setLayout(quickLayout);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(input, 1);
    inputLayout->addWidget(sendButton);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(exportCsvButton);
    buttonLayout->addWidget(exportAllButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(statusLabel);
    layout->addWidget(quickActions);
    layout->addWidget(history, 3);
    layout->addWidget(resultsLabel);
    layout->addWidget(resultsList, 1);
    layout->addLayout(inputLayout);
    layout->addLayout(buttonLayout);

    connect(sendButton, &QPushButton::clicked, this, &Form::onSendClicked);
    connect(input, &QLineEdit::returnPressed, this, &Form::onSendClicked);
    connect(resultsList, &QListWidget::itemActivated, this, &Form::onResultActivated);
    connect(clearButton, &QPushButton::clicked, this, &Form::onClearClicked);
    connect(exportCsvButton, &QPushButton::clicked, this, &Form::onExportCsvClicked);
    connect(exportAllButton, &QPushButton::clicked, this, &Form::onExportAllClicked);

    connect(btnErrors, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnWarnings, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnCan, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnTimeout, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnPattern, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnTimeline, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnSummary, &QPushButton::clicked, this, &Form::onQuickActionClicked);
    connect(btnHelp, &QPushButton::clicked, this, &Form::onQuickActionClicked);
}

void Form::appendMessage(const QString &author, const QString &html)
{
    const QString safeAuthor = author.toHtmlEscaped();
    history->append(QString("<p><b>%1:</b> %2</p>").arg(safeAuthor, html));
    QScrollBar *scrollBar = history->verticalScrollBar();
    if (scrollBar)
    {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void Form::setResults(const QList<int> &indices, const QStringList &snippets)
{
    resultsList->clear();

    const int count = qMin(indices.size(), snippets.size());
    for (int i = 0; i < count; ++i)
    {
        QListWidgetItem *item = new QListWidgetItem(resultsList);
        item->setText(QString("%1: %2").arg(indices[i]).arg(snippets[i]));
        item->setData(Qt::UserRole, indices[i]);
        resultsList->addItem(item);
    }
}

void Form::setStatusText(const QString &text)
{
    statusLabel->setText(text);
}

void Form::onSendClicked()
{
    const QString query = input->text().trimmed();
    if (query.isEmpty())
    {
        return;
    }

    input->clear();
    emit querySubmitted(query);
}

void Form::onResultActivated(QListWidgetItem *item)
{
    if (!item)
    {
        return;
    }

    bool ok = false;
    const int index = item->data(Qt::UserRole).toInt(&ok);
    if (ok)
    {
        emit indexActivated(index);
    }
}

void Form::onClearClicked()
{
    emit clearHighlightsRequested();
}

void Form::exportResultsToCsv(const QString &defaultFileName)
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Esporta Risultati in CSV"),
        defaultFileName.isEmpty() ? "dlt_results.csv" : defaultFileName,
        tr("CSV Files (*.csv);;All Files (*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    QList<int> indices;
    QStringList snippets;

    for (int i = 0; i < resultsList->count(); ++i)
    {
        QListWidgetItem *item = resultsList->item(i);
        if (item)
        {
            bool ok = false;
            int index = item->data(Qt::UserRole).toInt(&ok);
            if (ok)
            {
                indices.append(index);
                snippets.append(item->text());
            }
        }
    }

    emit exportRequested(filePath, indices, snippets, lastQuery);
}

void Form::exportAllToCsv(const QString &defaultFileName)
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Esporta Tutti i Log in CSV"),
        defaultFileName.isEmpty() ? "dlt_all_logs.csv" : defaultFileName,
        tr("CSV Files (*.csv);;All Files (*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    emit exportAllRequested(filePath);
}

void Form::onExportCsvClicked()
{
    exportResultsToCsv();
}

void Form::onExportAllClicked()
{
    exportAllToCsv();
}

void Form::onQuickActionClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn)
    {
        return;
    }

    QString query;
    if (btn == btnErrors)
    {
        query = "error";
    }
    else if (btn == btnWarnings)
    {
        query = "warn";
    }
    else if (btn == btnCan)
    {
        query = "can";
    }
    else if (btn == btnTimeout)
    {
        query = "timeout delay";
    }
    else if (btn == btnPattern)
    {
        query = "pattern ripeti";
    }
    else if (btn == btnTimeline)
    {
        query = "timeline";
    }
    else if (btn == btnSummary)
    {
        query = "summary";
    }
    else if (btn == btnHelp)
    {
        query = "help";
    }

    if (!query.isEmpty())
    {
        input->setText(query);
        onSendClicked();
    }
}

} // namespace DltChat
