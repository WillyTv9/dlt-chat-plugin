#ifndef DLTCHAT_RESULTS_MODEL_H
#define DLTCHAT_RESULTS_MODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QVector>
#include <QHash>

namespace dltchat {

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

    explicit ResultsModel(QObject *parent = nullptr);

    void setResults(const QList<int> &indices,
                    const QStringList &snippets,
                    const QStringList &levels,
                    const QList<QColor> &colors);

    void clearResults();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QList<int> allIndices() const { return m_indices; }

private:
    QList<int> m_indices;
    QStringList m_snippets;
    QStringList m_levels;
    QList<QColor> m_colors;
};

} // namespace dltchat

#endif
