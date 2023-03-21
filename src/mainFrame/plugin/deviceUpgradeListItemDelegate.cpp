#include "deviceUpgradeListItemDelegate.h"

#include "devicePluginInterface.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QRect>
#include <QImage>
#include <QSize>
#include <QDebug>

DeviceUpgradeListItemDelegate::DeviceUpgradeListItemDelegate(const DevPluginInterface *devPlugin, QObject *parent)
    : QStyledItemDelegate(parent)
    , devPlugin_(devPlugin)
{
    failureIcon_ = QPixmap(":/res/images/ic_failure_reason.svg");
    auto devType = devPlugin_->deviceType();
    if(devType == 0) {
        hardwareImage_ = QImage(":/res/images/ic_name_of_hareware.svg");
    } else if(devType == 1){
        hardwareImage_ = QImage(":/res/images/ic_yanshee.png").scaled(QSize(24,24),Qt::KeepAspectRatio);
    }
}

void DeviceUpgradeListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setPen(QColor(0xFF,0xFF,0xFF,0xCC));
    QRect paintRect = option.rect;
    int p = option.rect.width() / 10; //3:3:2:2

    int imageY = (paintRect.height() - hardwareImage_.size().height()) / 2;
    paintRect.adjust(20, imageY, 0, 0);
    paintRect.setWidth(hardwareImage_.width());
    paintRect.setHeight(hardwareImage_.height());
    painter->drawImage(paintRect, hardwareImage_);
//    painter->drawImage(0, imageY, hardwareImage_);
    QFont font = painter->font();
    font.setPixelSize(14);
    painter->setFont(font);
    paintRect = option.rect;
    paintRect.adjust(hardwareImage_.size().width()+20, 0, 0, 0);

    QString name = index.data(DeviceNameRole).toString();
    QString sn = index.data(DeviceSnRole).toString();
    if (name.isEmpty()) {
        name = sn.right(4);
    }
    painter->drawText(paintRect, Qt::AlignVCenter, name);
    paintRect = option.rect;
    paintRect.adjust(p * 3, 0, 0, 0);

    painter->drawText(paintRect, Qt::AlignVCenter, sn);
    paintRect = option.rect;
    paintRect.adjust(p * 6, 0, 0, 0);

    QString text = index.data(DeviceVersionRole).toString();
    painter->drawText(paintRect, Qt::AlignVCenter, text);
    paintRect = option.rect;
    paintRect.adjust(p * 8, 0, 0, 0);

//    painter->save();
    int upgradeConditions = index.data(DeviceUpgradeConditionsRole).toInt();
    UpgradePhase upgradePhase = static_cast<UpgradePhase>(index.data(DeviceUpgradePhaseRole).toInt());
    int transferProgress = index.data(DeviceFileTransferProgressRole).toInt();
    UpgradeResult upgradeResult =  static_cast<UpgradeResult>(index.data(DeviceUpgradeResultRole).toInt());
    text = DeviceUpgradeListItemDelegate::getStatusDisplayText(upgradePhase, upgradeConditions, transferProgress, upgradeResult);
    if ((upgradePhase == UpgradePhase::Finished && upgradeResult != UpgradeResult::Succeed)
            || (upgradePhase != UpgradePhase::Finished && !devPlugin_->meetUpgradeRequirements(upgradeConditions)))
    {
        painter->setPen(QColor(0xFF,0x2F,0x6F,0xCC));
    }
    painter->drawText(paintRect, Qt::AlignVCenter, text);
    if(upgradePhase == UpgradePhase::Finished && upgradeResult != UpgradeResult::Succeed) {
        paintRect.setX(option.rect.x() + option.rect.width() - 40);
        paintRect.setY(option.rect.y() + (option.rect.height()/2 - failureIcon_.height()/2));
        paintRect.setSize(failureIcon_.size());
        painter->drawPixmap(paintRect,failureIcon_);
    }
    painter->restore();
}

QString DeviceUpgradeListItemDelegate::upgradeConditionsDescription(int conditions) const
{
    return devPlugin_->upgradeConditionsDescription(conditions, *upgradePackageInfo);
}

QString DeviceUpgradeListItemDelegate::upgradePhaseDescription(UpgradePhase upgradePhase)
{
    switch(upgradePhase) {
    case UpgradePhase::Wait: {
        return tr("wait");
    }
    case UpgradePhase::TransferImage: {
        return tr("transfer image");
    }
    case UpgradePhase::Upgrading: {
        return tr("upgrading");
    }
    case UpgradePhase::Finished: {
        return tr("finished");
    }
    default:
        return tr("unknown");
    }
}

QString DeviceUpgradeListItemDelegate::upgradeResultDescription(int upgradeResult) const
{
    switch(upgradeResult) {
    case UpgradeResult::Unfinished: {
        return tr("unfinished"); //未完成就是失败
    }
    case UpgradeResult::Succeed: {
        return devPlugin_->upgradeSuccessText();
    }
    case UpgradeResult::Fail: {
        return tr("fail");
    }
    case UpgradeResult::TransferTimeout: {
        return tr("transfer timeout");
    }
    case UpgradeResult::UpgradeTimeout: {
        return tr("upgrade timeout");
    }
    case UpgradeResult::NotEnoughSpace: {
        return tr("no enough space");
    }
    case UpgradeResult::DevBusy: {
        return tr("device busy");
    }
    default:
        return tr("unknown");
    }
}

QString DeviceUpgradeListItemDelegate::getStatusDisplayText(UpgradePhase upgradePhase, int upgradeConditions, int imageTranferProgress, UpgradeResult upgradeResult) const
{
    if (UpgradePhase::Finished == upgradePhase) {
        return upgradeResultDescription(upgradeResult);
    } else {
        if (upgradePhase != UpgradePhase::Upgrading && !devPlugin_->meetUpgradeRequirements(upgradeConditions)) {
            return upgradeConditionsDescription(upgradeConditions);
        } else {
            if (upgradePhase == UpgradePhase::Wait) {
                return upgradeConditionsDescription(upgradeConditions);
            }
            else if (upgradePhase == UpgradePhase::PrepareTransfer) {
                return tr("prepare to transfer");
            }
            else if (upgradePhase == UpgradePhase::TransferImage) {
                if(imageTranferProgress == 0) {
                    return tr("prepare to transfer");
                }
                return DeviceUpgradeListItemDelegate::upgradePhaseDescription(upgradePhase) + QString(" %1%").arg(imageTranferProgress);
            } else { // UpgradePhase::Upgrading
                return DeviceUpgradeListItemDelegate::upgradePhaseDescription(upgradePhase);
            }
        }
    }
}
