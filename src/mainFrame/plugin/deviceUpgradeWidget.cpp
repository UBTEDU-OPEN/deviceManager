#include "deviceUpgradeWidget.h"
#include "ui_deviceUpgradeWidget.h"

#include "logHelper.h"
#include "commonDeviceItem.h"
#include "settings.h"
#include "deviceUpgradeThread.h"
#include "deviceUpgradeListItemDelegate.h"
#include "commondialog.h"
#include "warningwidget.h"
#include "devicemanagement.pb.h"

#include <QFile>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QTcpSocket>
#include <QPixmap>

const QString DeviceUpgradeWidget::QSS_PATH(":/res/qss/deviceUpgradeWidget.qss");
const int DeviceUpgradeWidget::ImportSelectPageIndex = 0;
const int DeviceUpgradeWidget::ImportProgressPageIndex = 1;

DeviceUpgradeWidget::DeviceUpgradeWidget(const DevPluginInterface *devPlugin, const QList<CommonDeviceItem*> &devicesToUpgrade, QWidget *parent)
    : QWidget(parent)
    , devPlugin_(devPlugin)
    , deviceItemModel_(nullptr)
    , devices_(devicesToUpgrade)
    , packageImportThread_(nullptr)
    , upgradeThread_(nullptr)
    , ui(new Ui::DeviceUpgradeWidget)
    , allFinished_(false)
    , upgradeStarted_(false)
    , importStarted_(false)
{
    ui->setupUi(this);

    setDefaultStyle();

    int i = 0;
    for(auto device : devices_) {
        LOG(INFO) << "check widget sn:" << device->sn().toStdString() << ",index:" << i;
        ++i;
    }

    init();
}

DeviceUpgradeWidget::~DeviceUpgradeWidget()
{
    if (upgradeThread_) {
        upgradeThread_->exitUpgrade();
        upgradeThread_->wait();
    }

    if (packageImportThread_) {
        packageImportThread_->stopImport();
        packageImportThread_->wait();
    }

    for (int r = 0; r < deviceItemModel_->rowCount(); ++r) {
        devices_[r]->stopFileTransfer();
        devices_[r]->setDeviceExecuting(false);
    }

    sendExitUpgradeToClient();

    delete ui;
}

void DeviceUpgradeWidget::setDefaultStyle()
{
    QFile styleSheet(QSS_PATH);
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    else {
        LOG(ERROR) << "DeviceUpgradeWidget::setDefaultStyle open qss failed";
    }
    QFont font = ui->FileNameLabel->font();
    font.setPixelSize(9);
    ui->FileNameLabel->setFont(font);
}

void DeviceUpgradeWidget::init()
{
    ui->UpgradeNoticeLabel->setText(devPlugin_->upgradeNoticeText());

    loadLocalUpgradePkgInfo();

    deviceItemModel_ = new QStandardItemModel(devices_.count(), 1, this);
    int row = 0;
//    QPixmap pixmap(":/res/images/ic_failure_reason.svg");
    for (auto device : devices_) {
        QStandardItem *deviceItem = new QStandardItem;
        deviceItemModel_->setItem(row++, deviceItem);
        deviceItem->setData(device->deviceName(), DeviceUpgradeListItemDelegate::DeviceNameRole);
        deviceItem->setData(device->sn(), DeviceUpgradeListItemDelegate::DeviceSnRole);
        deviceItem->setData(device->version(), DeviceUpgradeListItemDelegate::DeviceVersionRole);
        int upgradeConditions = 0;
        if (device->cmdSocketConnected()) {
            upgradeConditions |= DevPluginInterface::CmdConnect;
        }
        if (device->charging()) {
            upgradeConditions |= DevPluginInterface::Charging;
        }
        if (!device->locked()) {
            upgradeConditions |= DevPluginInterface::LockStatus;
        }
        QString upgradeConfirmNotice;
        if (devPlugin_->upgradeVersionCheck(device->version(), upgradePkgInfo_, upgradeConfirmNotice)) {
            upgradeConditions |= DevPluginInterface::Version;
        }
        deviceItem->setData(upgradeConditions, DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole);
        deviceItem->setData(0, DeviceUpgradeListItemDelegate::DeviceFileTransferProgressRole);
        deviceItem->setData(UpgradePhase::Wait, DeviceUpgradeListItemDelegate::DeviceUpgradePhaseRole);
        deviceItem->setData(UpgradeResult::Unfinished, DeviceUpgradeListItemDelegate::DeviceUpgradeResultRole);
        deviceItem->setData(upgradeConfirmNotice, DeviceUpgradeListItemDelegate::DeviceUpgradeConfirmNoticeRole);

        connect(device, &CommonDeviceItem::chargingStateChanged, this, &DeviceUpgradeWidget::onDevChargingStateChanged);
        connect(device, &CommonDeviceItem::lockedStateChanged, this, &DeviceUpgradeWidget::onDevLockStateChanged);
        connect(device, &CommonDeviceItem::versionChanged, this, &DeviceUpgradeWidget::onDevVersionChanged);
        connect(device, &CommonDeviceItem::cmdConnectionStateChanged, this, &DeviceUpgradeWidget::onDevCmdConnectionStateChanged);
        connect(device, &CommonDeviceItem::transferConnectionStateChanged, this, &DeviceUpgradeWidget::onDevTransferConnectionStateChanged);
        connect(device, &CommonDeviceItem::transferInterrupt, this, &DeviceUpgradeWidget::onDevTransferInterrupt);
        connect(device, &CommonDeviceItem::fileTransferProgressed, this, &DeviceUpgradeWidget::onDevFileTransferProgressed);
    }

    ui->DevLstView->setModel(deviceItemModel_);
    auto delegate = new DeviceUpgradeListItemDelegate(devPlugin_);
    delegate->upgradePackageInfo = &upgradePkgInfo_;
    ui->DevLstView->setItemDelegate(delegate);
    ui->DevLstView->setAlternatingRowColors(true);

    connect(ui->ImportBtn, &QPushButton::clicked, this, &DeviceUpgradeWidget::onImportClicked);
    connect(ui->ReselectPkgBtn, &QPushButton::clicked, this, &DeviceUpgradeWidget::onImportClicked);
    connect(ui->CancelImportingBtn, &QPushButton::clicked, this, &DeviceUpgradeWidget::onCancelImportClicked);

    connect(ui->UpgradeBtn, &QPushButton::clicked, this, &DeviceUpgradeWidget::onUpgradeClicked);
    connect(ui->StopUpgradeBtn, &QPushButton::clicked, this, &DeviceUpgradeWidget::onStopUpgradeClicked);
    connect(ui->DevLstView,&DeviceListView::failureIconClicked,this, &DeviceUpgradeWidget::onFailureIconClicked);
}

void DeviceUpgradeWidget::updateDeviceStatus2Lst()
{
    for (int r = 0; r < deviceItemModel_->rowCount(); ++r) {
        auto deviceItem = deviceItemModel_->item(r);
        deviceItem->setData(devices_[r]->deviceName(), DeviceUpgradeListItemDelegate::DeviceNameRole);
        deviceItem->setData(devices_[r]->sn(), DeviceUpgradeListItemDelegate::DeviceSnRole);
        deviceItem->setData(devices_[r]->version(), DeviceUpgradeListItemDelegate::DeviceVersionRole);
        int conditions = 0;
        if (devices_[r]->cmdSocketConnected()) {
            conditions |= DevPluginInterface::CmdConnect;
        }
        if (devices_[r]->charging()) {
            conditions |= DevPluginInterface::Charging;
        }
        if (!devices_[r]->locked()) {
            conditions |= DevPluginInterface::LockStatus;
        }
        QString upgradeConfirmNotice;
        if (devPlugin_->upgradeVersionCheck(devices_[r]->version(), upgradePkgInfo_, upgradeConfirmNotice)) {
            conditions |= DevPluginInterface::Version;
        }
        deviceItem->setData(conditions, DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole);
        deviceItem->setData(upgradeConfirmNotice, DeviceUpgradeListItemDelegate::DeviceUpgradeConfirmNoticeRole);
    }
}

void DeviceUpgradeWidget::loadLocalUpgradePkgInfo()
{
    ui->ReselectPkgBtn->setEnabled(true);
    ui->ImportSubContainer->setCurrentIndex(ImportSelectPageIndex);
    ui->UpgradeBtn->show();
    ui->UpgradeBtn->setEnabled(false);
    ui->StopUpgradeBtn->hide();
    QString upgradeFolderPath = Settings::deviceUpgradeFolderAbsPath(devPlugin_->name());
    QDir upgradeFolder(upgradeFolderPath);
    upgradePkgInfo_ = {0};
    if (upgradeFolder.exists()) {
        if (devPlugin_->readUpgradePackageInfo(upgradePkgInfo_)) {
            ui->ImportSubContainer->setCurrentIndex(ImportProgressPageIndex);
            ui->FileNameLabel->setText(upgradePkgInfo_.displayName);
            setNotice(true, upgradePkgInfo_.description);
            ui->ImportingProgressBar->hide();
            ui->ImportingProgressLabel->hide();
            ui->ReselectPkgBtn->show();
            ui->CancelImportingBtn->hide();
            ui->UpgradeBtn->setEnabled(true);
        } else {
            upgradeFolder.removeRecursively();
        }
    }
}

void DeviceUpgradeWidget::setNotice(bool success, const QString &info)
{
    ui->InfoIcon->setProperty("exception", !success);
    ui->InfoLabel->setProperty("exception", !success);
    ui->InfoLabel->setText(info);
    if(success) {
        ui->InfoIcon->setVisible(true);
        ui->warningIcon_2->setVisible(false);
    } else {
        ui->InfoIcon->setVisible(false);
        ui->warningIcon_2->setVisible(true);
    }

    ui->InfoLabel->style()->unpolish(ui->InfoLabel);
    ui->InfoLabel->style()->polish(ui->InfoLabel);
    ui->InfoIcon->style()->unpolish(ui->InfoIcon);
    ui->InfoIcon->style()->polish(ui->InfoIcon);
}

void DeviceUpgradeWidget::onImportClicked()
{
    QString upgradePkgPath = QFileDialog::getOpenFileName(this, tr("import upgrade package"), "", tr("Images (*.tar *.gz *.zip)"));
    if (upgradePkgPath.isEmpty()) {
        return;
    }
    ui->UpgradeBtn->setEnabled(false);

    packageImportThread_ = devPlugin_->createPackageImportThread(upgradePkgPath, this);
    connect(packageImportThread_, &PackageImportThreadInterface::finished, this, &DeviceUpgradeWidget::onPackageImportThreadFinished);
    connect(packageImportThread_, &PackageImportThreadInterface::importStart, this, &DeviceUpgradeWidget::onPackageImportStart);
    connect(packageImportThread_, &PackageImportThreadInterface::importProcessing, this, &DeviceUpgradeWidget::onPackageImportProcessing);
    connect(packageImportThread_, &PackageImportThreadInterface::importFinished, this, &DeviceUpgradeWidget::onPackageImportFinished);
    connect(packageImportThread_, &PackageImportThreadInterface::importCanceled, this, &DeviceUpgradeWidget::onPackageImportCanceled);
    packageImportThread_->start();
    QFileInfo upgradePkgInfo(upgradePkgPath);
    ui->FileNameLabel->setText(upgradePkgInfo.fileName());
}

void DeviceUpgradeWidget::onCancelImportClicked()
{
    if (packageImportThread_) {
        packageImportThread_->stopImport();
    }
    ui->InfoIcon->show();
    ui->InfoLabel->show();
    ui->ImportingProgressBar->hide();
    ui->ImportingProgressLabel->hide();
    ui->ReselectPkgBtn->show();
    ui->CancelImportingBtn->hide();

    loadLocalUpgradePkgInfo();
    updateDeviceStatus2Lst();
}

void DeviceUpgradeWidget::onPackageImportThreadFinished()
{
    if (packageImportThread_) {
        packageImportThread_->deleteLater();
        packageImportThread_ = nullptr;
    }
}

void DeviceUpgradeWidget::onPackageImportStart()
{
    ui->ImportSubContainer->setCurrentIndex(ImportProgressPageIndex);
    ui->InfoIcon->hide();
    ui->InfoLabel->hide();
    ui->ImportingProgressBar->show();
    ui->ImportingProgressBar->setValue(0);
    ui->ImportingProgressLabel->show();
    ui->ImportingProgressLabel->setText("0%");
    ui->ReselectPkgBtn->hide();
    ui->CancelImportingBtn->show();
    importStarted_ = true;
}

void DeviceUpgradeWidget::onPackageImportProcessing(int val)
{
    ui->ImportingProgressBar->setValue(val);
    ui->ImportingProgressLabel->setText(QString("%1%").arg(val));
}

void DeviceUpgradeWidget::onPackageImportFinished(bool success, QString errorStr)
{
    ui->InfoIcon->show();
    ui->InfoLabel->show();
    ui->ImportingProgressBar->hide();
    ui->ImportingProgressLabel->hide();
    ui->ReselectPkgBtn->show();
    ui->CancelImportingBtn->hide();

    setNotice(success, errorStr);
    if (!success) {
        CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
        QString upgradeWarningMsg = errorStr;
        warningwidget *warning = new warningwidget(upgradeWarningMsg,dialog);
        dialog->setDisplayWidget(warning);
        dialog->exec();
    }
    loadLocalUpgradePkgInfo();
    updateDeviceStatus2Lst();
    importStarted_ = false;
}

void DeviceUpgradeWidget::onPackageImportCanceled()
{
//    loadLocalUpgradePkgInfo();
    updateDeviceStatus2Lst();
    importStarted_ = false;
}

void DeviceUpgradeWidget::onUpgradeClicked()
{
//    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
//    QString upgradeWarningMsg = tr("In order to ensure the safe upgrade of the device, it is necessary to confirm that the device is connected to the power supply.\
//                                   And disconnect the power supply. After the upgrade, the device will lose the data previously kept on the device, please back up in advance.\
//                                   \nDetermine to perform the upgrade to version %1").arg(localPackageVersion_);
//    warningwidget *warning = new warningwidget(upgradeWarningMsg,dialog);
//    dialog->setDisplayWidget(warning);
//    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted) {
//        if (accepted) {
//            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, this);
//            SaveUserDataWidget *saveUserDataConfirmDlg = new SaveUserDataWidget(tr("After the upgrade, the device will lose the data previously retained on the device. If you need to retain the data, please back up the data to the userdata directory and check the retain userdata option."), dialog);
//            connect(saveUserDataConfirmDlg, &SaveUserDataWidget::sigAccepted, this, &DeviceUpgradeWidget::startUpgradeProcess);
//            dialog->setDisplayWidget(saveUserDataConfirmDlg);
//            connect(saveUserDataConfirmDlg, &SaveUserDataWidget::sigAccepted, dialog, &CommonDialog::close);
//            dialog->setFixedSize(580, 230);
//            dialog->show();
//        }
//    });
//    dialog->show();

    auto upgradeConfirmProcedure = devPlugin_->createUpgradeConfirmProcedure(this);
    if (devPlugin_->deviceType() == 1 && upgradePkgInfo_.diffPackage) {
        upgradeConfirmProcedure->hideSaveUserData();
    }
    connect(upgradeConfirmProcedure, &ConfirmProcedure::confirmComplete, this, &DeviceUpgradeWidget::startUpgradeProcess);
    upgradeConfirmProcedure->startProcedure();
}

void DeviceUpgradeWidget::onStopUpgradeClicked()
{
    if(allFinished_) {
        emit closeRequest();
        return;
    }
    warningwidget *warning = new warningwidget(tr("are you sure to stop upgrading?"));
    CommonDialog *dialog = new CommonDialog(tr("upgrade"), CommonDialog::OkCancelButton,this);
    connect(dialog,&CommonDialog::sigAccepted,[this](bool accepted){
        if(accepted) {
            emit closeRequest();
        }
    });
    dialog->setMinimumSize(580,170);
    dialog->setDisplayWidget(warning);
    dialog->show();
}

void DeviceUpgradeWidget::startUpgradeProcess(int accepted, int saveUserData)
{
    if (accepted) {
        QDir upgradeFolder(Settings::deviceUpgradeFolderAbsPath(devPlugin_->name()));
        QSettings configFile(upgradeFolder.filePath("config.ini"), QSettings::IniFormat);
        QString imageName = configFile.value("Config/imageName").toString();
        if (imageName.isEmpty()) {
            for (auto fileNameInUpgradeFolder : upgradeFolder.entryList(QDir::Files)) {
                if (fileNameInUpgradeFolder != "config.ini") {
                    imageName = fileNameInUpgradeFolder;
                    break;
                }
            }
        }
        QString version = configFile.value("Config/version").toString();
        QList<CommonDeviceItem*> devicesToUpgrade;
        QList<int> failDevicesIndex;
        QStringList upgradeConfirmNotices;
        for (int i = 0; i < deviceItemModel_->rowCount(); ++i) {
            if (auto deviceItem = deviceItemModel_->item(i)) {
                auto upgradeConditions = deviceItem->data(DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole).toInt();
                if (devPlugin_->meetUpgradeRequirements(upgradeConditions)) {
                    auto upgradeConfirmNotice = deviceItem->data(DeviceUpgradeListItemDelegate::DeviceUpgradeConfirmNoticeRole).toString();
                    if (!upgradeConfirmNotice.isNull() && !upgradeConfirmNotice.isEmpty()) {
                        upgradeConfirmNotice = tr("device %1 ").arg(deviceItem->data(DeviceUpgradeListItemDelegate::DeviceSnRole).toString()) + upgradeConfirmNotice;
                        upgradeConfirmNotices.append(upgradeConfirmNotice);
                    }
                    devicesToUpgrade.append(devices_[i]);
                } else {
                    failDevicesIndex.append(i);
                    LOG(INFO) << "device fail:" << devices_[i]->sn().toStdString();
                }
            }
        }
        if (devicesToUpgrade.count() == 0) {
            warningwidget *warning = new warningwidget(tr("no device meets upgrade requirements"));
            CommonDialog *dialog = new CommonDialog(tr("upgrade"), CommonDialog::OnlyOkButton,this);
            dialog->setMinimumSize(580,170);
            dialog->setDisplayWidget(warning);
            dialog->show();
            return;
        }
        if (imageName.isEmpty()) {
            warningwidget *warning = new warningwidget(tr("no valid upgrade image found"));
            CommonDialog *dialog = new CommonDialog(tr("upgrade"), CommonDialog::OnlyOkButton,this);
            dialog->setMinimumSize(580,170);
            dialog->setDisplayWidget(warning);
            dialog->show();
            return;
        }

        bool stopProcdure = false;
        if (upgradeConfirmNotices.count() != 0) {
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
            QString upgradeWarningMsg = upgradeConfirmNotices.join("\n") + tr("\n - continue to upgrade?");
            warningwidget *warning = new warningwidget(upgradeWarningMsg, dialog);
            dialog->setDisplayWidget(warning);
            connect(dialog, &CommonDialog::sigAccepted, [this, &stopProcdure](bool accepted) {
                stopProcdure = !accepted;
            });
            dialog->exec();
        }
        if (stopProcdure) {
            return;
        }

        if (upgradeThread_) {
            upgradeThread_->retryUpgrade(devices_);
        } else {
            qRegisterMetaType<QList<int>>("QList<int>");
            upgradeThread_ = new DeviceUpgradeThread(devPlugin_, devices_, imageName, version, saveUserData,failDevicesIndex);
            connect(upgradeThread_, &DeviceUpgradeThread::finished, this, &DeviceUpgradeWidget::onUpgradeThreadFinished);
            connect(upgradeThread_, &DeviceUpgradeThread::upgradeFinished, this, &DeviceUpgradeWidget::onUpgradeFinished);
            connect(upgradeThread_, &DeviceUpgradeThread::requestStartUpgradeProgcess, this, &DeviceUpgradeWidget::onStartUpgradeProgcessRequest);
            connect(upgradeThread_, &DeviceUpgradeThread::upgradePhaseChanged, this, &DeviceUpgradeWidget::onUpgradePhaseChanged);
            connect(upgradeThread_, &DeviceUpgradeThread::updateDeviceResult, this, &DeviceUpgradeWidget::onUpdateResult);
            upgradeThread_->start();
        }
        ui->StopUpgradeBtn->show();
        ui->StopUpgradeBtn->setEnabled(true);
        ui->UpgradeBtn->hide();
        ui->UpgradeBtn->setEnabled(false);
        ui->ReselectPkgBtn->setEnabled(false);
        upgradeStarted_ = true;
    } else {
        LOG(INFO) << "upgrade process does not start...";
    }
}

void DeviceUpgradeWidget::onUpgradeThreadFinished()
{
    if (upgradeThread_) {
        upgradeThread_->deleteLater();
        upgradeThread_ = nullptr;
    }
}

void DeviceUpgradeWidget::onUpgradeFinished(QList<int> upgradeRes)
{
    QStringList upgradeInfo;
    int succeedCount = 0;
    if (auto deviceUpgradeListItemDelegate = dynamic_cast<DeviceUpgradeListItemDelegate*>(ui->DevLstView->itemDelegate())) {
        for (int i = 0; i < upgradeRes.count(); ++i) {
            QString deviceUpgradeInfo = QString("%1 - %2")
                    .arg(deviceItemModel_->item(i)->data(DeviceUpgradeListItemDelegate::DeviceNameRole).toString())
                    .arg(deviceUpgradeListItemDelegate->upgradeResultDescription(deviceItemModel_->item(i)->data(DeviceUpgradeListItemDelegate::DeviceUpgradeResultRole).toInt()));
            if (upgradeRes.at(i) == UpgradeResult::Succeed) {
                ++succeedCount;
            }
            upgradeInfo.append(deviceUpgradeInfo);
        }
    }
    upgradeInfo.append(tr("succeed: %1 (total: %2)").arg(succeedCount).arg(upgradeRes.count()));
    warningwidget *warning = new warningwidget(upgradeInfo.join("\n"));
    CommonDialog *dialog = new CommonDialog(tr("upgrade result"), CommonDialog::OnlyOkButton,this);
    dialog->setMinimumSize(580,170);
    dialog->setDisplayWidget(warning);
    dialog->show();
    allFinished_ = true;
    ui->StopUpgradeBtn->setText(tr("upgrade finished"));
}

void DeviceUpgradeWidget::onStartUpgradeProgcessRequest(int index, QString imgName, QString version, int saveUserData)
{
    if (index < devices_.count()) {
        devices_[index]->onStartUpgradeProgcessRequest(upgradePkgInfo_, saveUserData);
    }
}

void DeviceUpgradeWidget::onUpgradePhaseChanged(int index, int upgradePhase)
{
    LOG(INFO) << "index=" << index << " upgrade phase=" << upgradePhase;
    deviceItemModel_->item(index)->setData(upgradePhase, DeviceUpgradeListItemDelegate::DeviceUpgradePhaseRole);
    if(upgradePhase != UpgradePhase::Finished) {
        upgradingDeviceIndexSet_.insert(index);
    } else {
        if(upgradingDeviceIndexSet_.contains(index)) {
            upgradingDeviceIndexSet_.remove(index);
        }
    }
}

void DeviceUpgradeWidget::onUpdateResult(int index, int result)
{
    LOG(INFO) << "index=" << index << " result=" << result;
    deviceItemModel_->item(index)->setData(result, DeviceUpgradeListItemDelegate::DeviceUpgradeResultRole);
    setDeviceStatusCode(index, result);
}

void DeviceUpgradeWidget::onDevCmdConnectionStateChanged(bool /*newState*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
            int upgradeConditions = deviceItemModel_->item(index)->data(DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole).toInt();
            if (dev->cmdSocketConnected()) {
                upgradeConditions |= DevPluginInterface::CmdConnect;
            } else {
                upgradeConditions &= ~DevPluginInterface::CmdConnect;
            }
            deviceItemModel_->item(index)->setData(upgradeConditions, DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole);
        }
    }
}

void DeviceUpgradeWidget::onDevTransferConnectionStateChanged(bool /*newState*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
        }
    }
}

void DeviceUpgradeWidget::onDevTransferInterrupt()
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
        }
    }
}

void DeviceUpgradeWidget::onDevVersionChanged(QString /*oldVersion*/, QString /*newVersion*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
            int upgradeConditions = deviceItemModel_->item(index)->data(DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole).toInt();
            QString upgradeConfirmNotice;
            if (devPlugin_->upgradeVersionCheck(dev->version(), upgradePkgInfo_, upgradeConfirmNotice)) {
                upgradeConditions |= DevPluginInterface::Version;
            }
            deviceItemModel_->item(index)->setData(upgradeConditions, DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole);
            deviceItemModel_->item(index)->setData(upgradeConfirmNotice, DeviceUpgradeListItemDelegate::DeviceUpgradeConfirmNoticeRole);
        }
    }
}

void DeviceUpgradeWidget::onDevChargingStateChanged(bool /*newStatus*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
            int upgradeConditions = deviceItemModel_->item(index)->data(DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole).toInt();
            if (dev->charging()) {
                upgradeConditions |= DevPluginInterface::Charging;
            } else {
                upgradeConditions &= ~DevPluginInterface::Charging;
            }
            deviceItemModel_->item(index)->setData(upgradeConditions, DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole);
        }
    }
}

void DeviceUpgradeWidget::onDevLockStateChanged(bool /*newLockState*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
            int upgradeConditions = deviceItemModel_->item(index)->data(DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole).toInt();
            if (!dev->locked()) {
                upgradeConditions |= DevPluginInterface::LockStatus;
            } else {
                upgradeConditions &= ~DevPluginInterface::LockStatus;
            }
            deviceItemModel_->item(index)->setData(upgradeConditions, DeviceUpgradeListItemDelegate::DeviceUpgradeConditionsRole);
        }
    }
}

void DeviceUpgradeWidget::onDevFileTransferProgressed(qint64 /*value*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        int index = devices_.indexOf(dev);
        if (index != -1) {
            deviceItemModel_->item(index)->setData(dev->fileTransferProgress(), DeviceUpgradeListItemDelegate::DeviceFileTransferProgressRole);
        }
    }
}

void DeviceUpgradeWidget::setDeviceStatusCode(int index, int resultCode)
{
    switch (resultCode) {
    case UpgradeResult::TransferTimeout:
        devices_[index]->status = DeviceStatusCode::kImgTransferFailed;
        break;
    case UpgradeResult::UpgradeTimeout:
        devices_[index]->status = DeviceStatusCode::kUpgradeTimeout;
        break;
    case UpgradeResult::NotEnoughSpace:
        devices_[index]->status = DeviceStatusCode::kUpgradeNoEnoughSpace;
        break;
    case UpgradeResult::Fail:
        devices_[index]->status = DeviceStatusCode::kUpgradeFail;
        break;
    case UpgradeResult::DevBusy:
        devices_[index]->status = DeviceStatusCode::kDevBusy;
        break;
    case UpgradeResult::DevObsoleteVersion:
        devices_[index]->status = DeviceStatusCode::kDevObsoleteVersion;
        break;
    default:
        break;
    }
}

void DeviceUpgradeWidget::onFailureIconClicked(int index)
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
    QString warningMsg;
    switch (devices_[index]->status) {
    case DeviceStatusCode::kImgTransferFailed:
    {
        warningMsg = tr("transfer failed, please check the network.");
    }
        break;
    case DeviceStatusCode::kUpgradeTimeout:
    {
        warningMsg = tr("upgrade timeout.");
    }
        break;
    case DeviceStatusCode::kUpgradeNoEnoughSpace:
    {
        warningMsg = tr("no enough space, please clear and retry.");
    }
        break;
    default:
        warningMsg = tr("Upgrade unknown error.");
        break;
    }
    warningwidget *warning = new warningwidget(warningMsg,dialog);
    dialog->setDisplayWidget(warning);
    dialog->setOkBtnText(tr("retry"));
    dialog->setMinimumSize(580,130);
    connect(dialog, &CommonDialog::sigAccepted, [this,index](bool accepted){
        if(accepted) {
            devices_[index]->status = DeviceStatusCode::kNormalState;
            allFinished_ = false;
            ui->StopUpgradeBtn->setText(tr("stop upgrade"));
            upgradeThread_->retry(devices_[index]);
        }
    });
    dialog->show();
}

void DeviceUpgradeWidget::sendExitUpgradeToClient()
{
    dm::CmdPackage cmdPkg;
    cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_CANCELUPGRADE);
    dm::TransMsg transPkg;
    transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
    transPkg.set_packagecontent(cmdPkg.SerializeAsString());
    auto data = QByteArray::fromStdString(transPkg.SerializeAsString());

    if (upgradeStarted_) {
        for (int r = 0; r < deviceItemModel_->rowCount(); ++r) {
            auto deviceItem = deviceItemModel_->item(r);
    //        auto result = static_cast<UpgradeResult>(deviceItem->data(DeviceUpgradeListItemDelegate::DeviceUpgradeResultRole).toInt());
            auto phase = static_cast<UpgradePhase>(deviceItem->data(DeviceUpgradeListItemDelegate::DeviceUpgradePhaseRole).toInt());
            if (phase != UpgradePhase::Finished &&
                    devices_[r]->cmd == dm::CmdPackage_CMDTYPE_UPGRADE && //已经发送过升级指令：
                    devices_[r]->deviceConnected()) {
                devices_[r]->cmdSocket->write(data);
                devices_[r]->cmdSocket->flush();
            }
        }
    }
}

void DeviceUpgradeWidget::onClose()
{
    if(importStarted_) {
        warningwidget *warning = new warningwidget(tr("importing image, are you sure to close?"));
        CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton,this);
        connect(dialog,&CommonDialog::sigAccepted,[this](bool accepted){
            if(accepted) {
                emit closeRequest();
            }
        });
        dialog->setDisplayWidget(warning);
        dialog->show();
    } else if(upgradeStarted_) {
        onStopUpgradeClicked();
    } else {
        emit closeRequest();
    }
}
