#ifndef DLTCHAT_RESULTS_MODEL_H
#define DLTCHAT_RESULTS_MODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QColor>
#include <functional>

namespace dltchat {

struct LogEntryData {
    QString snippet;
    QString level;
    QColor color;
};

class ResultsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        SnippetRole,
        LevelRole,
        ColorRole
    };

    typedef std::function<LogEntryData(int logIndex)> DataFetcher;

    explicit ResultsModel(QObject *parent = nullptr);

    void setDataFetcher(DataFetcher fetcher) { m_fetcher = fetcher; }

    void setResults(const QList<int> &indices);
    void clearResults();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QList<int> allIndices() const { return m_indices; }

private:
    QList<int> m_indices;
    DataFetcher m_fetcher;
};

} // namespace dltchat

#endif
