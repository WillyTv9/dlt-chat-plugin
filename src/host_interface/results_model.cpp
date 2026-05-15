#include "results_model.h"
#include <QColor>

namespace dltchat {

ResultsModel::ResultsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ResultsModel::setResults(const QList<int> &indices)
{
    beginResetModel();
    m_indices = indices;
    endResetModel();
}

void ResultsModel::clearResults()
{
    beginResetModel();
    m_indices.clear();
    endResetModel();
}

int ResultsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_indices.size();
}

QVariant ResultsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_indices.size())
        return QVariant();

    int row = index.row();
    int logIndex = m_indices[row];

    if (role == IndexRole) return logIndex;

    if (!m_fetcher) return QVariant();

    LogEntryData d = m_fetcher(logIndex);

    if (role == Qt::DisplayRole) {
        return QString("%1: %2").arg(logIndex).arg(d.snippet);
    }
    if (role == SnippetRole) return d.snippet;
    if (role == LevelRole) return d.level;
    if (role == ColorRole) return d.color;

    return QVariant();
}

} // namespace dltchat
