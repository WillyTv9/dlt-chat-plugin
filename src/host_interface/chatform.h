#ifndef CHATFORM_H
#define CHATFORM_H

#include <QWidget>
#include <QHash>
#include <QListView>
#include <QStringList>
#include <QVector>
#include <QPair>
#include <QColor>

#include "results_model.h"
class QLabel;
class QTextBrowser;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QGridLayout;

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

    //! UI description of one macro-category drop-down: a label plus its
    //! (native filter name, highlight colour) entries. Built by the host plugin
    //! from the .dlp catalog and pushed in via buildNativeFilterMenus().
    struct NativeMenuSpec {
        QString label;
        QVector<QPair<QString, QColor>> actions;
    };

    explicit Form(QWidget *parent = nullptr);
    void buildNativeFilterMenus(const QVector<NativeMenuSpec> &specs);
    void appendMessage(const QString &author, const QString &html);
    void appendMessage(MessageRole role, const QString &html);
    static QString roleLabel(MessageRole role);
    void setResults(const QList<int> &indices);
    void setDataFetcher(dltchat::ResultsModel::DataFetcher fetcher) { if (m_resultsModel) m_resultsModel->setDataFetcher(fetcher); }
    void setAiStatus(int state, const QString &modelName = QString());
    QList<int> cachedQuickActionResult(const QString &query) const { return m_quickActionCache.value(query); }
    void storeQuickActionResult(const QString &query, const QList<int> &indices) { m_quickActionCache.insert(query, indices); }
    void clearQuickActionCache() { m_quickActionCache.clear(); }

public slots:
    void setStatusText(const QString &text);
    void setProcessingProgress(bool visible);
    void exportResultsToCsv(const QString &def = QString());
    void exportAllToCsv(const QString &def = QString());

signals:
    void querySubmitted(const QString &query);
    void quickActionTriggered(const QString &query);
    void nativeFilterTriggered(const QString &filterName);
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
    QHash<QString, QList<int>> m_quickActionCache;
    QWidget *m_nativeMenuBar = nullptr;
    QGridLayout *m_nativeMenuLayout = nullptr;
    QLabel *m_nativeMenuPlaceholder = nullptr;
};

} // namespace DltChat

#endif
