#include "results_model.h"
#include <QColor>

namespace dltchat {

ResultsModel::ResultsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ResultsModel::setResults(const QList<int> &indices,
                               const QStringList &snippets,
                               const QStringList &levels,
                               const QList<QColor> &colors)
{
    beginResetModel();
    m_indices = indices;
    m_snippets = snippets;
    m_levels = levels;
    m_colors = colors;
    endResetModel();
}

void ResultsModel::clearResults()
{
    beginResetModel();
    m_indices.clear();
    m_snippets.clear();
    m_levels.clear();
    m_colors.clear();
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

    if (role == Qt::DisplayRole) {
        QString snippet = (row < m_snippets.size()) ? m_snippets[row] : QString();
        return QString("%1: %2").arg(m_indices[row]).arg(snippet);
    }
    if (role == IndexRole) return m_indices[row];
    if (role == SnippetRole && row < m_snippets.size()) return m_snippets[row];
    if (role == LevelRole && row < m_levels.size()) return m_levels[row];
    if (role == ColorRole && row < m_colors.size()) return m_colors[row];

    return QVariant();
}

} // namespace dltchat
