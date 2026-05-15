#ifndef CHATFORM_H
#define CHATFORM_H

#include <QWidget>
#include <QHash>
#include <QListView>
#include <QStringList>

#include "results_model.h"
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
    enum class MessageRole {
        User,
        UserAi,
        Assistant,
        AiAssistant,
        AiAssistantFallback
    };

    explicit Form(QWidget *parent = nullptr);
    void appendMessage(const QString &author, const QString &html);
    void appendMessage(MessageRole role, const QString &html);
    static QString roleLabel(MessageRole role);
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
    void userFilterLoadRequested(const QString &path);

private slots:
    void onSendClicked();
    void onAiSendClicked();
    void onResultActivated(const QModelIndex &index);
    void onClearClicked();
    void onExportCsvClicked();
    void onExportAllClicked();
    void onQuickActionClicked();
    void onFilterLoadClicked();

private:
    QLabel *title;
    QLabel *statusLabel;
    QLabel *aiStatusLabel;
    QPushButton *configButton;
    QTextBrowser *history;
    QListView *resultsList;
    QLineEdit *input;
    QPushButton *sendButton;
    QLineEdit *aiInput;
    QPushButton *aiSendButton;
    QProgressBar *aiProgress;
    QPushButton *clearButton;
    QPushButton *exportCsvButton;
    QPushButton *exportAllButton;
    QPushButton *filterLoadButton;
    QLabel *aiDisclaimer;
    dltchat::ResultsModel *m_resultsModel;
    QString lastQuery;
    QHash<QObject *, QString> m_quickActionQueries;
};

} // namespace DltChat

#endif
