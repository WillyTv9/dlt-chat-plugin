#ifndef DLTCHAT_HIGHLIGHT_DELEGATE_H
#define DLTCHAT_HIGHLIGHT_DELEGATE_H

#include <QStyledItemDelegate>
#include <QHash>
#include <QColor>
#include <QPointer>

namespace DltChat {

//! Paints per-row background colours on the host main table view.
/*!
    DLT-Viewer's plugin interface exposes no API to inject marker filters into the
    host filter list, and ARTIST8 2.28 lacks QDltFile::setManualMarkerIndices().
    This delegate is the portable alternative: it keeps a row -> QColour map and
    fills the cell background for highlighted rows, giving each Quick Action its own
    .dlp filterColour. Cells without an entry are forwarded to the table's original
    delegate (if any), so native rendering is preserved when nothing is highlighted.
*/
class HighlightDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit HighlightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    //! Delegate to forward to for non-highlighted cells (typically the host's original).
    void setInnerDelegate(QAbstractItemDelegate *inner) { m_inner = inner; }

    //! Replace the highlight map (key = source-model row, value = background colour).
    void setRowColors(const QHash<int, QColor> &colors);
    void clear();
    bool isEmpty() const { return m_rowColors.isEmpty(); }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    QHash<int, QColor>               m_rowColors;
    QPointer<QAbstractItemDelegate>  m_inner;
};

} // namespace DltChat

#endif // DLTCHAT_HIGHLIGHT_DELEGATE_H
