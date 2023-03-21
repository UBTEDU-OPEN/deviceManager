#include "devicelistview.h"

#include <QMouseEvent>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QPixmap>
#include <QDebug>

#include "deviceUpgradeListItemDelegate.h"
#include "commondialog.h"

DeviceListView::DeviceListView(QWidget *parent)
    : QListView(parent)
{
//    setMouseTracking(true);
    failureIconSize_ = QPixmap(":/res/images/ic_failure_reason.svg").size();
}

void DeviceListView::mousePressEvent(QMouseEvent* event)
{
    auto index = indexAt(event->pos());
    if(event->button() == Qt::LeftButton && pressOnFailureIcon(event->pos())) {
        emit failureIconClicked(index.row());
    }
}

void DeviceListView::mouseMoveEvent(QMouseEvent* event)
{
    QListView::mouseMoveEvent(event);
}

bool DeviceListView::pressOnFailureIcon(QPoint pos)
{
    auto index = indexAt(pos);
    auto standardModel = dynamic_cast<QStandardItemModel*>(model());
    if(standardModel) {
        auto standardItem = standardModel->item(index.row());
        if(standardItem) {
            UpgradeResult resultCode = static_cast<UpgradeResult>(standardItem->data(DeviceUpgradeListItemDelegate::DeviceUpgradeResultRole).toInt());
            UpgradePhase upgradePhase = static_cast<UpgradePhase>(standardItem->data(DeviceUpgradeListItemDelegate::DeviceUpgradePhaseRole).toInt());
            if(upgradePhase == UpgradePhase::Finished && resultCode != UpgradeResult::Succeed) {
                auto rect = visualRect(index);
                rect.setX(rect.x() + rect.width() - 40);
                rect.setY(rect.y() + (rect.height()/2 - failureIconSize_.height()/2));
                rect.setSize(failureIconSize_);
                return rect.contains(pos);
            }
        }
    }
    return false;
}
