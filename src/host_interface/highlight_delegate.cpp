#include "highlight_delegate.h"

#include <QPainter>
#include <QStyle>
#include <QPalette>
#include <QBrush>

namespace DltChat {

void HighlightDelegate::setRowColors(const QHash<int, QColor> &colors)
{
    m_rowColors = colors;
}

void HighlightDelegate::clear()
{
    m_rowColors.clear();
}

void HighlightDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    const QColor bg = m_rowColors.value(index.row(), QColor());

    // No highlight for this row: keep the host's native rendering intact.
    if (!bg.isValid()) {
        if (m_inner) { m_inner->paint(painter, option, index); return; }
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);

    // Let the standard selection highlight win over our colour when the row is
    // selected, otherwise paint the .dlp filterColour as the cell background.
    if (!(opt.state & QStyle::State_Selected)) {
        painter->save();
        painter->fillRect(opt.rect, bg);
        painter->restore();

        // Adjust the palette so the base/inner delegate draws text on top of the
        // colour rather than over its own (now overwritten) background brush.
        opt.backgroundBrush = QBrush(bg);
        const int luminance = (bg.red() * 299 + bg.green() * 587 + bg.blue() * 114) / 1000;
        const QColor fg = (luminance < 128) ? Qt::white : Qt::black;
        opt.palette.setColor(QPalette::Text, fg);
        opt.palette.setColor(QPalette::WindowText, fg);
    }

    if (m_inner) m_inner->paint(painter, opt, index);
    else         QStyledItemDelegate::paint(painter, opt, index);
}

QSize HighlightDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    if (m_inner) return m_inner->sizeHint(option, index);
    return QStyledItemDelegate::sizeHint(option, index);
}

} // namespace DltChat
