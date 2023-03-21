#include "sharedFileListItemDelegate.h"

#include <QFileSystemModel>
#include <QObject>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QRect>
#include <QEvent>
#include <QMouseEvent>
#include <QDateTime>
#include <QToolTip>
#include <QDebug>

SharedFileListItemDelegate::SharedFileListItemDelegate(QFileSystemModel *fileSystemModel, QObject *parent)
    : QStyledItemDelegate(parent)
    , fileSystemModel_(fileSystemModel)
{
}

void SharedFileListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
//    QStyledItemDelegate::paint(painter, option, index);
    painter->save();
    auto pen = painter->pen();
    pen.setColor(QColor(255,255,255,204));
    painter->setPen(pen);
    QRect drawTextRect = option.rect;
    if (fileSystemModel_) {
        drawTextRect.adjust(20, 0, -drawTextRect.width() / 2 - 5, 0);
        painter->drawText(drawTextRect, Qt::AlignVCenter | Qt::AlignLeft, fileSystemModel_->fileInfo(index).fileName());
        drawTextRect = option.rect.adjusted(option.rect.width() / 2, 0, 0, 0);
        painter->drawText(drawTextRect, Qt::AlignVCenter | Qt::AlignLeft, fileSystemModel_->fileInfo(index).birthTime().toString("yyyy/MM/dd hh:mm"));
    }
    drawTextRect = option.rect.adjusted(option.rect.width() * 5 / 6, 0, 0, 0);
    painter->drawText(drawTextRect, Qt::AlignVCenter | Qt::AlignLeft, tr("delete"));
    painter->restore();
}

bool SharedFileListItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    auto me = dynamic_cast<QMouseEvent*>(event);
    if (me && me->button() & Qt::LeftButton && me->type() == QEvent::MouseButtonRelease) {
        QRect deleteTextRect = option.rect.adjusted(option.rect.width() * 4 / 5, 0, 0, 0);
        if (deleteTextRect.contains(me->pos())) {
            emit deleteRequest(index);
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

