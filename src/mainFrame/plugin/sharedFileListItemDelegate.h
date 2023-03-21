#ifndef SHAREDFILELISTITEMDELEGATE_H
#define SHAREDFILELISTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTimer>

class QFileSystemModel;
class QStyleOptionViewItem;
class QModelIndex;
class QPainter;
class QAbstractItemModel;
class QObject;

class SharedFileListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SharedFileListItemDelegate(QFileSystemModel *fileSystemModel, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

signals:
    void deleteRequest(const QModelIndex index);

private:
    QFileSystemModel *fileSystemModel_;
};

#endif // SHAREDFILELISTITEMDELEGATE_H
