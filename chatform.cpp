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

namespace DltChat {

Form::Form(QWidget *parent)
    : QWidget(parent)
    , statusLabel(new QLabel(this))
    , history(new QTextBrowser(this))
    , resultsList(new QListWidget(this))
    , input(new QLineEdit(this))
    , sendButton(new QPushButton(tr("Invia"), this))
    , clearButton(new QPushButton(tr("Pulisci evidenziazioni"), this))
    , exportCsvButton(new QPushButton(tr("Esporta Risultati CSV"), this))
    , exportAllButton(new QPushButton(tr("Esporta Tutto CSV"), this))
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

    input->setPlaceholderText(tr("Fai una domanda sui log (es. 'mostra errori')"));

    QLabel *resultsLabel = new QLabel(tr("Risultati"), this);

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

} // namespace DltChat
