#ifndef DEVICEUPGRADETHREAD_H
#define DEVICEUPGRADETHREAD_H

#include "CommonDeviceItem.h"

#include <QThread>
#include <QMap>
#include <QList>
#include <QMutex>

enum UpgradePhase {
    Wait,
    PrepareTransfer,
    TransferImage,
    Upgrading,
    Finished
};

enum UpgradeResult {
    Unfinished,
    Succeed,
    Fail,
    TransferTimeout,
    UpgradeTimeout,
    NotEnoughSpace,
    DevBusy,
    DevObsoleteVersion,
};

struct UpgradeStatus {
    UpgradePhase phase;
    UpgradeResult result;
    int prepareTransferWaitTimes;
    int transferTriedTimes;
    int upgradeTimeout;
};

class DevPluginInterface;

class DeviceUpgradeThread : public QThread
{
    Q_OBJECT

public:
    int upgradeTimeLimit;
    int transferAttemptionTimesLimit;
    int parallelTransferDeviceCountLimit;
    int transferContinuousAttemptionTimesLimit;
    int prepareTransferWaitTimesLimit;

public:
    explicit DeviceUpgradeThread(const DevPluginInterface *devPlugin,
                                 const QList<CommonDeviceItem*> &devicesToUpgrade,
                                 const QString &imageName, const QString &version, int saveUserData,
                                 QList<int> failDevicesIndex,
                                 QObject *parent = nullptr);
    virtual ~DeviceUpgradeThread();

    void run() override;

    void pauseUpgrade();
    void retryUpgrade(const QList<CommonDeviceItem*> &devices =  QList<CommonDeviceItem*>());
    void exitUpgrade();

    void onCmdConnectionStateChanged(bool connect);
    void onTransferConnectionStateChanged(bool connect);
    void onTransferStart();
    void onTransferInterrupt();
    void onUpgradeResultBroadcast(bool success);
    void onDeviceAboutToUpgrade(int code);

    void retry(CommonDeviceItem* device);

private:
    void tryToStartTranfer(int deviceIndex, int &devicesCountCanStartTransfer);

signals:
    void upgradePhaseChanged(int index, int upgradePhase);
    void updateDeviceResult(int index, int result);
    void upgradeFinished(QList<int> upgradeResult);
    void requestStartUpgradeProgcess(int index,
                                     QString imgName, QString version,
                                     int saveUserData);

private:
    const DevPluginInterface *devPlugin_;

    QString imageName_;
    QString version_;
    int saveUserData_;

    bool exitRequested_;
    bool waitRequested_;

    QList<CommonDeviceItem*> devices_;
    QList<UpgradeStatus> upgradeStatus_;
    QList<int> failDevicesIndex_;

    QMutex mutex_;
};

#endif // DEVICEUPGRADETHREAD_H
