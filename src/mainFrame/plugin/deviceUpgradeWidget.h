#ifndef DEVICEUPGRADEWIDGET_H
#define DEVICEUPGRADEWIDGET_H

#include "devicePluginInterface.h"

#include <QWidget>
#include <QSet>

class PackageImportThread;
class DeviceUpgradeThread;
class CommonDeviceItem;
class QStandardItemModel;

namespace Ui {
class DeviceUpgradeWidget;
}

class DeviceUpgradeWidget : public QWidget
{
    Q_OBJECT

private:
    static const QString QSS_PATH;
    static const int ImportSelectPageIndex;
    static const int ImportProgressPageIndex;

public:
    explicit DeviceUpgradeWidget(const DevPluginInterface *devPlugin, const QList<CommonDeviceItem*> &devicesToUpgrade, QWidget *parent = nullptr);
    ~DeviceUpgradeWidget();

    void onClose();

protected:
    void setDefaultStyle();

private:
    void init();
    void updateDeviceStatus2Lst();

    void loadLocalUpgradePkgInfo();
    void setNotice(bool success, const QString &info = "");

    void onImportClicked();
    void onCancelImportClicked();
    void onPackageImportThreadFinished();
    void onPackageImportStart();
    void onPackageImportProcessing(int val);
    void onPackageImportFinished(bool success, QString errorStr);
    void onPackageImportCanceled();

    void onUpgradeClicked();
    void onStopUpgradeClicked();
    void startUpgradeProcess(int accepted, int saveUserData);
    void onUpgradeThreadFinished();
    void onUpgradeFinished(QList<int> upgradeRes);
    void onStartUpgradeProgcessRequest(int index,
                                       QString imgName, QString version,
                                       int saveUserData);
    void onUpgradePhaseChanged(int index, int upgradePhase);
    void onUpdateResult(int index, int result);
    void onDevCmdConnectionStateChanged(bool newState);
    void onDevTransferConnectionStateChanged(bool newState);
    void onDevTransferInterrupt();
    void onDevVersionChanged(QString oldVersion, QString newVersion);
    void onDevChargingStateChanged(bool newStatus);
    void onDevLockStateChanged(bool newLockState);
    void onDevFileTransferProgressed(qint64 value);
    void setDeviceStatusCode(int index, int resultCode);
    void onFailureIconClicked(int index);
    void sendExitUpgradeToClient();

signals:
    void closeRequest();

private:
    const DevPluginInterface *devPlugin_;

    Ui::DeviceUpgradeWidget *ui;

    QStandardItemModel *deviceItemModel_;
    QList<CommonDeviceItem*> devices_;
    UpgradePackageInfo upgradePkgInfo_;

    PackageImportThreadInterface *packageImportThread_;
    DeviceUpgradeThread *upgradeThread_;
    QSet<int> upgradingDeviceIndexSet_;
    bool allFinished_;
    bool upgradeStarted_;
    bool importStarted_;
};

#endif // DEVICEUPGRADEWIDGET_H
