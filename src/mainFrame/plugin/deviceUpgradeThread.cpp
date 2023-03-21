#include "deviceUpgradeThread.h"

#include "logHelper.h"
#include "settings.h"
#include "devicemanagement.pb.h"
#include "devicePluginInterface.h"

#include <QMutexLocker>

DeviceUpgradeThread::DeviceUpgradeThread(const DevPluginInterface *devPlugin,
                                         const QList<CommonDeviceItem *> &devicesToUpgrade,
                                         const QString &imageName, const QString &version, int saveUserData,
                                         QList<int> failDevicesIndex,
                                         QObject *parent)
    : QThread(parent)
    , devPlugin_(devPlugin)
    , imageName_(imageName)
    , version_(version)
    , saveUserData_(saveUserData)
    , exitRequested_(false)
    , waitRequested_(false)
    , failDevicesIndex_(failDevicesIndex)
{
    LOG(INFO) << "DeviceUpgradeThread construct";
//    upgradeTimeLimit_ = Config::getUpgradeUpgradeTime();
//    transferAttemptionLimit_ = Config::getUpgradeTransferTimes();
//    parallelTransferDeviceCountLimit_ = Config::getUpgradeMaxDeviceCount();

    upgradeTimeLimit = devPlugin->upgradeTimeLimit();
    transferAttemptionTimesLimit = devPlugin->transferAttemptionTimesLimit();
    parallelTransferDeviceCountLimit = devPlugin->parallelTransferDeviceCountLimit();
    transferContinuousAttemptionTimesLimit = devPlugin->transferContinuousAttemptionTimesLimit();
    prepareTransferWaitTimesLimit = 10;

    LOG(INFO) << "upgradeTimeLimit_:" << upgradeTimeLimit;
    LOG(INFO) << "transferAttemptionLimit_:" << transferAttemptionTimesLimit;
    LOG(INFO) << "transferContinuousAttemptionLimit_:" << transferContinuousAttemptionTimesLimit;
    LOG(INFO) << "parallelTransferDeviceCountLimit_:"<< parallelTransferDeviceCountLimit;

    for (auto device : devicesToUpgrade) {
        UpgradeStatus initStatus;
        initStatus.phase = UpgradePhase::Wait;
        initStatus.prepareTransferWaitTimes = 0;
        initStatus.transferTriedTimes = 0;
        initStatus.result = UpgradeResult::Unfinished;
        initStatus.upgradeTimeout = 0;
        devices_.append(device);
        upgradeStatus_.append(initStatus);
        connect(device, &CommonDeviceItem::cmdConnectionStateChanged, this, &DeviceUpgradeThread::onCmdConnectionStateChanged);
        connect(device, &CommonDeviceItem::transferConnectionStateChanged, this, &DeviceUpgradeThread::onTransferConnectionStateChanged);
        connect(device, &CommonDeviceItem::transferInterrupt, this, &DeviceUpgradeThread::onTransferInterrupt);
        connect(device, &CommonDeviceItem::upgradeResultBroadcast, this, &DeviceUpgradeThread::onUpgradeResultBroadcast);
        connect(device, &CommonDeviceItem::aboutToUpgrading, this, &DeviceUpgradeThread::onDeviceAboutToUpgrade);
        connect(device, &CommonDeviceItem::transferStart, this, &DeviceUpgradeThread::onTransferStart);
    }

    int i = 0;
    for(auto device : devices_) {
        LOG(INFO) << "check thread sn:" << device->sn().toStdString() << ",index:" << i;
        ++i;
    }
}

DeviceUpgradeThread::~DeviceUpgradeThread()
{
    LOG(INFO) << "DeviceUpgradeThread destroy";
}

void DeviceUpgradeThread::run()
{
    LOG(INFO) << "DeviceUpgradeThread::run start";
    do
    {
        {
            QMutexLocker locker(&mutex_);
            if (exitRequested_) {
                LOG(INFO) << "exit request";
                break;
            } else if (waitRequested_) {
                LOG(INFO) << "upgrade finished, wait for next operation";
            } else {
                LOG(INFO) << "iterate devices status";
                int finishedCount = 0;
                QMap<int, int> transferTriedTimes2deviceIndex;
                QSet<int> attemptingTransferDevices;
                int transferingDeviceCount = 0;
                for (int i = 0; i < devices_.count(); ++i) {
                    UpgradeStatus status = upgradeStatus_[i];
                    switch (status.phase) {
                    case UpgradePhase::Wait: {
                        if (!failDevicesIndex_.empty() &&
                                failDevicesIndex_.contains(i)) {
                            upgradeStatus_[i].upgradeTimeout = 0;
                            upgradeStatus_[i].phase = UpgradePhase::Finished;
                            upgradeStatus_[i].result = UpgradeResult::Fail;
                            emit updateDeviceResult(i, UpgradeResult::Fail);
                            LOG(INFO) << devices_[i]->sn().toStdString() << " fail to start upgrade";
                            devices_[i]->setDeviceExecuting(false);
                            emit upgradePhaseChanged(i, UpgradePhase::Finished);
                        }
                        else if (upgradeStatus_[i].transferTriedTimes % transferContinuousAttemptionTimesLimit != 0) {
                            attemptingTransferDevices.insert(i);
                        } else {
                            transferTriedTimes2deviceIndex.insertMulti(upgradeStatus_[i].transferTriedTimes, i);
                        }
                    }
                        break;
                    case UpgradePhase::PrepareTransfer: {
//                        transferTriedTimes2deviceIndex.insertMulti(upgradeStatus_[i].transferTriedTimes, i);
                        ++transferingDeviceCount;
                        upgradeStatus_[i].prepareTransferWaitTimes += 1;
                        if (upgradeStatus_[i].prepareTransferWaitTimes >= prepareTransferWaitTimesLimit) {
                            CommonDeviceItem *device = devices_[i];
                            LOG(INFO) << "device prepare transfer timeout, sn:" << device->sn().toStdString() << " prepareTransferWaitTimes:" << upgradeStatus_[i].prepareTransferWaitTimes;
                            upgradeStatus_[i].prepareTransferWaitTimes = 0;
                            upgradeStatus_[i].phase = UpgradePhase::Wait;
                            emit upgradePhaseChanged(i, UpgradePhase::Wait);
                        }
                    }
                        break;
                    case UpgradePhase::TransferImage: {
                        ++transferingDeviceCount;
                    }
                        break;
                    case UpgradePhase::Upgrading: {
                        if (upgradeStatus_[i].upgradeTimeout < upgradeTimeLimit) {
                            upgradeStatus_[i].upgradeTimeout += 1;
                        } else {
                            upgradeStatus_[i].upgradeTimeout = 0;
                            upgradeStatus_[i].phase = UpgradePhase::Finished;
                            upgradeStatus_[i].result = UpgradeResult::UpgradeTimeout;
                            emit updateDeviceResult(i, UpgradeResult::UpgradeTimeout);
                            LOG(INFO) << devices_[i]->sn().toStdString() << " upgrade time out";
                            devices_[i]->setDeviceExecuting(false);
                            emit upgradePhaseChanged(i, UpgradePhase::Finished);
                        }
                    }
                        break;
                    case UpgradePhase::Finished: {
                        ++finishedCount;
                    }
                        break;
                    }
                }

                int devicesCountCanStartTransfer = parallelTransferDeviceCountLimit - transferingDeviceCount;
                LOG(INFO) << "devicesCountCanStartTransfer:" << devicesCountCanStartTransfer << " transferingDeviceCount:" << transferingDeviceCount;
                if (devicesCountCanStartTransfer > 0) {
                    for (auto attemptingTransferDevice : attemptingTransferDevices) {
                        tryToStartTranfer(attemptingTransferDevice, devicesCountCanStartTransfer);
                    }

                    auto triedTimes = transferTriedTimes2deviceIndex.uniqueKeys();
                    for (auto itr = triedTimes.begin(); itr != triedTimes.end(); ++itr) {
                        auto deviceIndices = transferTriedTimes2deviceIndex.values(*itr);
                        for (auto indexItr = deviceIndices.rbegin(); indexItr != deviceIndices.rend(); ++indexItr) {
                            tryToStartTranfer(*indexItr, devicesCountCanStartTransfer);
                        }
                    }
                }

                if (finishedCount == devices_.count()) {
                    LOG(INFO) << "----------- upgrade finished:";
                    waitRequested_ = true;
                    QList<int> upgradeRes;
                    for (int idx = 0; idx < upgradeStatus_.count(); ++idx) {
                        upgradeRes.append(static_cast<int>(upgradeStatus_[idx].result));
                        LOG(INFO) << "device " << devices_[idx]->sn().toStdString()
                                  << " transferTriedTimes:" << upgradeStatus_[idx].transferTriedTimes
                                  << " upgradeTimeout:" << upgradeStatus_[idx].upgradeTimeout;
                    }
                    emit upgradeFinished(upgradeRes);
                }
            }
        }
        QThread::sleep(1);
    } while (true);
    LOG(INFO) << "DeviceUpgradeThread::run finished";
}

void DeviceUpgradeThread::pauseUpgrade()
{
    LOG(INFO) << "DeviceUpgradeThreadpauseUpgrade";
    QMutexLocker locker(&mutex_);
    waitRequested_ = true;
}

void DeviceUpgradeThread::retryUpgrade(const QList<CommonDeviceItem *> &devices)
{
    LOG(INFO) << "DeviceUpgradeThread::retryUpgrade, devices count = " << devices.count();
    QMutexLocker locker(&mutex_);
    if (devices.isEmpty()) {
        for (int i = 0; i < devices_.count(); ++i) {
//            if (upgradeStatus_[i].phase == UpgradePhase::Finished && upgradeStatus_[i].result != UpgradeResult::Succeed) {
                upgradeStatus_[i].phase = UpgradePhase::Wait;
                upgradeStatus_[i].prepareTransferWaitTimes = 0;
                upgradeStatus_[i].transferTriedTimes = 0;
                upgradeStatus_[i].result = UpgradeResult::Unfinished;
                upgradeStatus_[i].upgradeTimeout = 0;
                emit upgradePhaseChanged(i, UpgradePhase::Wait);
//            }
        }
    } else {
        for (int i = 0; i < upgradeStatus_.count(); ++i) {
            upgradeStatus_[i].phase = UpgradePhase::Wait;
            upgradeStatus_[i].prepareTransferWaitTimes = 0;
            upgradeStatus_[i].transferTriedTimes = 0;
            upgradeStatus_[i].result = UpgradeResult::Unfinished;
            upgradeStatus_[i].upgradeTimeout = 0;
            emit upgradePhaseChanged(i, UpgradePhase::Wait);
        }
    }
    waitRequested_ = false;
}

void DeviceUpgradeThread::exitUpgrade()
{
    LOG(INFO) << "DeviceUpgradeThread::exitUpgrade";
    QMutexLocker locker(&mutex_);
    exitRequested_ = true;
}

void DeviceUpgradeThread::onCmdConnectionStateChanged(bool connect)
{
    LOG(INFO) << "DeviceUpgradeThread::onCmdConnectionStateChanged, connect " << connect;
    QMutexLocker locker(&mutex_);
    if (auto device = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(device);
        LOG(INFO) << "DeviceUpgradeThread::onCmdConnectionStateChanged sn:" << device->sn().toStdString() << " upgrade phase:" << upgradeStatus_[index].phase;
        switch(upgradeStatus_[index].phase) {
        case UpgradePhase::Wait: {
        }
            break;
        case UpgradePhase::PrepareTransfer: {
            if (connect) {

            } else {
                upgradeStatus_[index].phase = UpgradePhase::Wait;
                emit upgradePhaseChanged(index, UpgradePhase::Wait);
            }
        }
            break;
        case UpgradePhase::TransferImage: {
        }
            break;
        case UpgradePhase::Upgrading: {
        }
            break;
        case UpgradePhase::Finished: {
        }
            break;
        }
    }
}

void DeviceUpgradeThread::onTransferConnectionStateChanged(bool connect)
{
    LOG(INFO) << "DeviceUpgradeThread::onTransferConnectionStateChanged, connect " << connect;
    QMutexLocker locker(&mutex_);
    if (auto device = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(device);
        LOG(INFO) << "DeviceUpgradeThread::onTransferConnectionStateChanged sn:" << device->sn().toStdString() << " upgrade phase:" << upgradeStatus_[index].phase;
        switch(upgradeStatus_[index].phase) {
        case UpgradePhase::Wait: {
        }
            break;
        case UpgradePhase::PrepareTransfer: {
        }
            break;
        case UpgradePhase::TransferImage: {
            if (connect) {

            } else if (device->fileTransferProgress() < 100) {
                upgradeStatus_[index].phase = UpgradePhase::Wait;
                emit upgradePhaseChanged(index, UpgradePhase::Wait);
            } else {
                upgradeStatus_[index].phase = UpgradePhase::Upgrading;
                emit upgradePhaseChanged(index, UpgradePhase::Upgrading);
            }
        }
            break;
        case UpgradePhase::Upgrading: {
        }
            break;
        case UpgradePhase::Finished: {
        }
            break;
        }
    }
}

void DeviceUpgradeThread::onTransferStart()
{
    LOG(INFO) << "DeviceUpgradeThread::onTransferStart";
    QMutexLocker locker(&mutex_);
    if (auto device = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(device);
        LOG(INFO) << "DeviceUpgradeThread::onTransferStart sn:" << device->sn().toStdString()
                  << " upgrade phase:" << upgradeStatus_[index].phase
                  << " upgrade process:" << device->fileTransferProgress();
        switch(upgradeStatus_[index].phase) {
        case UpgradePhase::Wait: {
        }
            break;
        case UpgradePhase::PrepareTransfer: {
//            upgradeStatus_[index].transferTriedTimes = 0;
            upgradeStatus_[index].phase = UpgradePhase::TransferImage;
            emit upgradePhaseChanged(index, UpgradePhase::TransferImage);
        }
            break;
        case UpgradePhase::TransferImage: {
        }
            break;
        case UpgradePhase::Upgrading: {
        }
            break;
        case UpgradePhase::Finished: {
        }
            break;
        }
    }
}

void DeviceUpgradeThread::onTransferInterrupt()
{
    LOG(INFO) << "DeviceUpgradeThread::onTransferInterrupt";
    QMutexLocker locker(&mutex_);
    if (auto device = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(device);
        LOG(INFO) << "DeviceUpgradeThread::onTransferInterrupt sn:" << device->sn().toStdString()
                  << " upgrade phase:" << upgradeStatus_[index].phase
                  << " upgrade process:" << device->fileTransferProgress();
        switch(upgradeStatus_[index].phase) {
        case UpgradePhase::Wait: {
        }
            break;
        case UpgradePhase::PrepareTransfer: {
        }
            break;
        case UpgradePhase::TransferImage: {
            if (device->fileTransferProgress() < 100) {
                LOG(INFO) << "change phase to UpgradePhase::Wait";
                upgradeStatus_[index].phase = UpgradePhase::Wait;
                emit upgradePhaseChanged(index, UpgradePhase::Wait);
            } else {
                LOG(INFO) << "change phase to UpgradePhase::Upgrading";
                upgradeStatus_[index].phase = UpgradePhase::Upgrading;
                emit upgradePhaseChanged(index, UpgradePhase::Upgrading);
            }
        }
            break;
        case UpgradePhase::Upgrading: {
        }
            break;
        case UpgradePhase::Finished: {
        }
            break;
        }
    }
}

void DeviceUpgradeThread::onUpgradeResultBroadcast(bool success)
{
    LOG(INFO) << "DeviceUpgradeThread::onUpgradeResultBroadcast, success " << success;
    QMutexLocker locker(&mutex_);
    if (auto device = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(device);
        LOG(INFO) << "DeviceUpgradeThread::onUpgradeResultBroadcast sn:" << device->sn().toStdString() << " upgrade phase:" << upgradeStatus_[index].phase << " success:" << success;
        switch(upgradeStatus_[index].phase) {
        case UpgradePhase::Wait: {
        }
            break;
        case UpgradePhase::PrepareTransfer: {
        }
            break;
        case UpgradePhase::TransferImage: {
        }
            break;
        case UpgradePhase::Upgrading: {
            upgradeStatus_[index].phase = UpgradePhase::Finished;
            if (success) {
                upgradeStatus_[index].result = UpgradeResult::Succeed;
                emit updateDeviceResult(index, UpgradeResult::Succeed);
                emit upgradePhaseChanged(index, UpgradePhase::Finished);
            } else {
                upgradeStatus_[index].result = UpgradeResult::Fail;
                emit updateDeviceResult(index, UpgradeResult::Fail);
                emit upgradePhaseChanged(index, UpgradePhase::Finished);
            }
        }
            break;
        case UpgradePhase::Finished: {
        }
            break;
        }
    }
}

void DeviceUpgradeThread::onDeviceAboutToUpgrade(int code)
{
    LOG(INFO) << "DeviceUpgradeThread::onDeviceAboutToUpgrade";
    QMutexLocker locker(&mutex_);
    if (auto device = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(device);
        LOG(INFO) << "DeviceUpgradeThread::onDeviceAboutToUpgrade sn:" << device->sn().toStdString() << " upgrade phase:" << upgradeStatus_[index].phase << " code:" << code;
        if (code == ExceptionCode::FileTransferSucc || code == ExceptionCode::FileTransferAlreadyExists) {
            upgradeStatus_[index].phase = UpgradePhase::Upgrading;
            upgradeStatus_[index].result = UpgradeResult::Unfinished;
            upgradeStatus_[index].upgradeTimeout = 0;
            emit upgradePhaseChanged(index, UpgradePhase::Upgrading);
        }
        else if (code == ExceptionCode::FileTransferFail) {
            device->setDeviceExecuting(false);
            upgradeStatus_[index].phase = UpgradePhase::Finished;
            upgradeStatus_[index].result = UpgradeResult::DevBusy;
            emit updateDeviceResult(index, UpgradeResult::DevBusy);
            upgradeStatus_[index].upgradeTimeout = 0;
            emit upgradePhaseChanged(index, UpgradePhase::Finished);
        }
        else if (code == ExceptionCode::NoEnoughSpace) {
            device->setDeviceExecuting(false);
            upgradeStatus_[index].phase = UpgradePhase::Finished;
            upgradeStatus_[index].result = UpgradeResult::NotEnoughSpace;
            emit updateDeviceResult(index, UpgradeResult::NotEnoughSpace);
            upgradeStatus_[index].upgradeTimeout = 0;
            emit upgradePhaseChanged(index, UpgradePhase::Finished);
        }
        else if (code == ExceptionCode::DeviceBusy) {
            device->setDeviceExecuting(false);
            upgradeStatus_[index].phase = UpgradePhase::Finished;
            upgradeStatus_[index].result = UpgradeResult::DevBusy;
            emit updateDeviceResult(index, UpgradeResult::DevBusy);
            upgradeStatus_[index].upgradeTimeout = 0;
            emit upgradePhaseChanged(index, UpgradePhase::Finished);
        }
        else if (code == ExceptionCode::ObsoleteVersion) {
            device->setDeviceExecuting(false);
            upgradeStatus_[index].phase = UpgradePhase::Finished;
            upgradeStatus_[index].result = UpgradeResult::DevObsoleteVersion;
            emit updateDeviceResult(index, UpgradeResult::DevObsoleteVersion);
            upgradeStatus_[index].upgradeTimeout = 0;
            emit upgradePhaseChanged(index, UpgradePhase::Finished);
        }
        else if (code == ExceptionCode::UpgradeFail) {
            device->setDeviceExecuting(false);
            upgradeStatus_[index].phase = UpgradePhase::Finished;
            upgradeStatus_[index].result = UpgradeResult::Fail;
            emit updateDeviceResult(index, UpgradeResult::Fail);
            upgradeStatus_[index].upgradeTimeout = 0;
            emit upgradePhaseChanged(index, UpgradePhase::Finished);
        }
    }
}

void DeviceUpgradeThread::retry(CommonDeviceItem *device)
{
    LOG(INFO) << "DeviceUpgradeThread::retry";
    QMutexLocker locker(&mutex_);
    int index = devices_.indexOf(device);
    LOG(INFO) << "DeviceUpgradeThread::retry sn:" << device->sn().toStdString() << " upgrade phase:" << upgradeStatus_[index].phase;
    if (upgradeStatus_[index].phase == UpgradePhase::Finished) {
        upgradeStatus_[index].phase = UpgradePhase::Wait;
        upgradeStatus_[index].prepareTransferWaitTimes = 0;
        upgradeStatus_[index].transferTriedTimes = 0;
        upgradeStatus_[index].result = UpgradeResult::Unfinished;
        upgradeStatus_[index].upgradeTimeout = 0;
        emit upgradePhaseChanged(index, UpgradePhase::Wait);
        waitRequested_ = false;
    }
}

void DeviceUpgradeThread::tryToStartTranfer(int deviceIndex, int &devicesCountCanStartTransfer)
{
    CommonDeviceItem *device = devices_[deviceIndex];
    LOG(INFO) << "tryToStartTranfer sn:" << device->sn().toStdString()
              << " transferTriedTimes:" << upgradeStatus_[deviceIndex].transferTriedTimes
              << " devicesCountCanStartTransfer:" << devicesCountCanStartTransfer;
    if (upgradeStatus_[deviceIndex].transferTriedTimes >= transferAttemptionTimesLimit) {
        LOG(INFO) << "transfer timeout, sn:" << device->sn().toStdString();
        upgradeStatus_[deviceIndex].phase = UpgradePhase::Finished;
        upgradeStatus_[deviceIndex].result = UpgradeResult::TransferTimeout;
        emit updateDeviceResult(deviceIndex, UpgradeResult::TransferTimeout);
        emit upgradePhaseChanged(deviceIndex, UpgradePhase::Finished);
    }
    else if (devicesCountCanStartTransfer > 0) {
        LOG(INFO) << "prepare transfer, sn:" << device->sn().toStdString();
        upgradeStatus_[deviceIndex].transferTriedTimes += 1;
        --devicesCountCanStartTransfer;
        if (device->cmdSocketConnected()) {
            upgradeStatus_[deviceIndex].phase = UpgradePhase::PrepareTransfer;
            emit upgradePhaseChanged(deviceIndex, UpgradePhase::PrepareTransfer);
            emit requestStartUpgradeProgcess(deviceIndex, imageName_, version_, saveUserData_);
        } else {
            LOG(INFO) << "device cmd socket disconnected, sn:" << device->sn().toStdString();
        }
    }
}

