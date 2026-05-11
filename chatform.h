#ifndef CHATFORM_H
#define CHATFORM_H

#include <QWidget>
#include <QListWidget>

class QLabel;
class QTextBrowser;
class QLineEdit;
class QPushButton;
class QStringList;

namespace DltChat {

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    void appendMessage(const QString &author, const QString &html);
    void setResults(const QList<int> &indices, const QStringList &snippets);

public slots:
    void setStatusText(const QString &text);
    void exportResultsToCsv(const QString &defaultFileName = QString());
    void exportAllToCsv(const QString &defaultFileName = QString());

signals:
    void querySubmitted(const QString &query);
    void indexActivated(int index);
    void clearHighlightsRequested();
    void exportRequested(const QString &filePath, const QList<int> &indices, const QStringList &snippets, const QString &query);
    void exportAllRequested(const QString &filePath);

private slots:
    void onSendClicked();
    void onResultActivated(QListWidgetItem *item);
    void onClearClicked();
    void onExportCsvClicked();
    void onExportAllClicked();
    void onQuickActionClicked();

private:
    QLabel *statusLabel;
    QTextBrowser *history;
    QListWidget *resultsList;
    QLineEdit *input;
    QPushButton *sendButton;
    QPushButton *clearButton;
    QPushButton *exportCsvButton;
    QPushButton *exportAllButton;
    QPushButton *btnErrors;
    QPushButton *btnWarnings;
    QPushButton *btnCan;
    QPushButton *btnTimeout;
    QPushButton *btnPattern;
    QPushButton *btnTimeline;
    QPushButton *btnSummary;
    QPushButton *btnHelp;
    QString lastQuery;
};

} // namespace DltChat

#endif // CHATFORM_H
