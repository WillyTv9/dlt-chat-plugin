#ifndef CHATFORM_H
#define CHATFORM_H

#include <QWidget>
#include <QListWidget>
#include <QStringList>

class QLabel;
class QTextBrowser;
class QLineEdit;
class QPushButton;
class QProgressBar;

namespace DltChat {

class Form : public QWidget
{
    Q_OBJECT
public:
    explicit Form(QWidget *parent = nullptr);
    void appendMessage(const QString &author, const QString &html);
    void setResults(const QList<int> &indices, const QStringList &snippets,
                    const QStringList &levels = QStringList());
    void setAiStatus(int state, const QString &modelName = QString());

public slots:
    void setStatusText(const QString &text);
    void setProcessingProgress(bool visible);
    void exportResultsToCsv(const QString &def = QString());
    void exportAllToCsv(const QString &def = QString());

signals:
    void querySubmitted(const QString &query);
    void aiQuerySubmitted(const QString &query);
    void configureAiClicked();
    void indexActivated(int index);
    void clearHighlightsRequested();
    void exportRequested(const QString &path, const QList<int> &indices,
                         const QStringList &snippets, const QString &query);
    void exportAllRequested(const QString &path);

private slots:
    void onSendClicked();
    void onAiSendClicked();
    void onResultActivated(QListWidgetItem *item);
    void onClearClicked();
    void onExportCsvClicked();
    void onExportAllClicked();
    void onQuickActionClicked();

private:
    QLabel *title;
    QLabel *statusLabel;
    QLabel *aiStatusLabel;
    QPushButton *configButton;
    QTextBrowser *history;
    QListWidget *resultsList;
    QLineEdit *input;
    QPushButton *sendButton;
    QLineEdit *aiInput;
    QPushButton *aiSendButton;
    QProgressBar *aiProgress;
    QPushButton *clearButton;
    QPushButton *exportCsvButton;
    QPushButton *exportAllButton;
    QString lastQuery;
};

} // namespace DltChat

#endif
