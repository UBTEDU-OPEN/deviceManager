#ifndef DEVICEUPGRADELISTITEMDELEGATE_H
#define DEVICEUPGRADELISTITEMDELEGATE_H

#include "deviceUpgradeThread.h"
#include "devicePluginInterface.h"

#include <QStyledItemDelegate>
#include <QPixmap>
#include <QString>

class DevPluginInterface;

class QPainter;
class QStyleOptionViewItem;
class QModelIndex;


class DeviceUpgradeListItemDelegate  : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum ItemDataRole {
        DeviceNameRole = Qt::UserRole + 1,
        DeviceSnRole,
        DeviceVersionRole,
        DeviceUpgradeConditionsRole,
        DeviceFileTransferProgressRole,
        DeviceUpgradePhaseRole,
        DeviceUpgradeResultRole,
        DeviceUpgradeConfirmNoticeRole,
        Count
    };

    const UpgradePackageInfo *upgradePackageInfo;

public:
    explicit DeviceUpgradeListItemDelegate(const DevPluginInterface *devPlugin, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QString upgradeConditionsDescription(int conditions) const;
    static QString upgradePhaseDescription(UpgradePhase upgradePhase);
    QString upgradeResultDescription(int upgradeResult) const;
    QString getStatusDisplayText(UpgradePhase upgradePhase, int upgradeConditions,
                                 int imageTranferProgress, UpgradeResult upgradeResult) const;

private:
    const DevPluginInterface *devPlugin_;
    QPixmap failureIcon_;
    QImage hardwareImage_;
};

#endif // DEVICEUPGRADELISTITEMDELEGATE_H
