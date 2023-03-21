#include "deviceUpgradeDlg.h"
#include "ui_deviceUpgradeDlg.h"

#include "config.h"
#include "fileCopyThread.h"
#include "md5.h"
#include "fileDirHandler.h"
#include "logHelper.h"
#include "commonDeviceItem.h"
#include "saveUserDataConfirmDlg.h"
#include "commondialog.h"
#include "warningwidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QLineEdit>
#include <QStringList>
#include <QSettings>
#include <QString>
#include <QProcess>
#include <QTableWidget>
#include <QTableWidgetItem>

PackageImportThread::PackageImportThread(const QString &packagePath, QObject *parent)
    : QThread(parent)
    , packagePath_(packagePath)
    , extractProcess_(nullptr)
    , cancelRequest_(false)
{}

void PackageImportThread::run()
{
    emit importStart();

    QString version;
    QString md5InConfig;
    QString fileMd5;

    QString upgradeFolderPath = Config::upgradeFolderAbsPath();
    QDir upgradeFolder(upgradeFolderPath);
    if (!upgradeFolder.exists() && !upgradeFolder.mkpath(".")) {
        emit importFinished(false, tr("make upgrade dir failed"));
        return;
    }
    upgradeFolder.removeRecursively();
    if (cancelCheckPoint(10)) { return; }

    QString extractCmd = "7za.exe";
    QStringList extractArguments;
    extractArguments.append("e");
    extractArguments.append("-o" + upgradeFolder.absolutePath());
    extractArguments.append(packagePath_);
    extractProcess_ = new QProcess(this);
    extractProcess_->start(extractCmd, extractArguments);
    extractProcess_->waitForFinished(-1);

    QSettings configFile(upgradeFolder.filePath("config.ini"), QSettings::IniFormat);
    version = configFile.value("Config/version").toString();
    md5InConfig = configFile.value("Config/MD5").toString();
    LOG(INFO) << "version:" << version.toStdString();
    LOG(INFO) << "md5InConfig:" << md5InConfig.toStdString();
    if (version.isEmpty()) {
        upgradeFolder.removeRecursively();
        emit importFinished(false, tr("invalid version"));
        return;
    }
    if (cancelCheckPoint(50)) { return; }

    QFileInfo imgFileInfo;
    for (auto &fileInfo : upgradeFolder.entryInfoList(QDir::Files)) {
        if (!fileInfo.fileName().endsWith("config.ini")) {
            imgFileInfo = fileInfo;
            break;
        }
    }
    QString imgFilePath = imgFileInfo.absoluteFilePath();
    fileMd5 = MD5::fileMd5(imgFilePath);
    LOG(INFO) << "fileMd5:" << fileMd5.toStdString();
    if (md5InConfig.isEmpty() || fileMd5 != md5InConfig) {
        upgradeFolder.removeRecursively();
        emit importFinished(false, tr("invalid md5"));
        return;
    }
    if (cancelCheckPoint(80)) { return; }

    QFile imgFile(imgFilePath);
    imgFilePath = imgFilePath.replace(imgFileInfo.baseName(), fileMd5);
    imgFileInfo = QFileInfo(imgFilePath);
    if (!imgFileInfo.exists() && !imgFile.rename(imgFilePath)) {
        upgradeFolder.removeRecursively();
        emit importFinished(false, tr("rename image failed"));
        return;
    }
    if (cancelCheckPoint(90)) { return; }

    QFileInfo packageFileInfo(packagePath_);
    configFile.setValue("Config/name", packageFileInfo.fileName());
    imgFileInfo.setFile(imgFilePath);
    configFile.setValue("Config/imageName", imgFileInfo.fileName());
    configFile.sync();

    emit importFinished(true, "");
}

void PackageImportThread::stopImport()
{
    if (extractProcess_) {
        extractProcess_->kill();
        extractProcess_->deleteLater();
        extractProcess_ = nullptr;
    }
    cancelRequest_ = true;
}

bool PackageImportThread::cancelCheckPoint(int value)
{
    if (cancelRequest_) {
        QString upgradeFolderPath = Config::upgradeFolderAbsPath();
        QDir upgradeFolder(upgradeFolderPath);
        upgradeFolder.removeRecursively();
        emit importCanceled();
        return true;
    } else {
        emit importProcessing(value);
        return false;
    }
}

DeviceUpgradeDlg::DeviceUpgradeDlg(const QList<CommonDeviceItem*> &devicesToUpgrade, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DeviceUpgradeDlg)
    , packageImportThread_(nullptr)
    , upgradeThread_(nullptr)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("upgrade"));
    ui->stopImportPushButton->hide();
    ui->infoLabel->hide();
    ui->importProgressBar->setValue(0);
    ui->importProgressBar->hide();
    ui->upgradeImgLineEdit->setReadOnly(true);
    ui->stopUpgradePushButton->hide();

    connect(ui->importPushButton, &QPushButton::clicked, this, &DeviceUpgradeDlg::onImportClicked);
    connect(ui->stopImportPushButton, &QPushButton::clicked, this, &DeviceUpgradeDlg::onStopImportClicked);

    readUpgradePackageInfo();
    initDeviceList(devicesToUpgrade);

    connect(ui->upgradePushButton, &QPushButton::clicked, this, &DeviceUpgradeDlg::onUpgradeClicked);
    connect(ui->stopUpgradePushButton, &QPushButton::clicked, this, &DeviceUpgradeDlg::onStopUpgradeClicked);
}

DeviceUpgradeDlg::~DeviceUpgradeDlg()
{
    if (upgradeThread_) {
        upgradeThread_->exitUpgrade();
    }
    for (auto device : devices_) {
        device->stopFileTransfer();
        device->setDeviceExecuting(false);
    }
    delete ui;
}

void DeviceUpgradeDlg::initDeviceList(const QList<CommonDeviceItem*> &devicesToUpgrade)
{
    ui->deviceTableWidget->setColumnCount(Count);
    QStringList horizontalHeaderLabels;
    horizontalHeaderLabels.append(tr("name"));
    horizontalHeaderLabels.append(tr("sn"));
    horizontalHeaderLabels.append(tr("version"));
    horizontalHeaderLabels.append(tr("status"));
    horizontalHeaderLabels.append(tr("file transfer"));
    horizontalHeaderLabels.append(tr("upgrade phase"));
    horizontalHeaderLabels.append(tr("upgrade result"));
    horizontalHeaderLabels.append(tr("retry"));
    ui->deviceTableWidget->setHorizontalHeaderLabels(horizontalHeaderLabels);

    ui->deviceTableWidget->setRowCount(devicesToUpgrade.count());
    devices_.clear();
    for (int row = 0; row < devicesToUpgrade.count(); ++row) {
        CommonDeviceItem *device = devicesToUpgrade[row];
        devices_.append(device);
        for (int col = DeviceName; col < Count; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem;
            ui->deviceTableWidget->setItem(row, col, item);
            switch(col) {
            case DeviceName: {
                item->setText(device->deviceName());
            }
                break;
            case DeviceSn: {
                item->setText(device->sn);
            }
                break;
            case DeviceVersion: {
                item->setText(device->version());
            }
                break;
            case DeviceUpgradeRequirements: {
                int upgradeRequirements = 0;
                if (device->cmdSocketConnected()) {
                    upgradeRequirements |= UpgradeRequirements::CmdConnected;
                }
                if (device->charging()) {
                    upgradeRequirements |= UpgradeRequirements::Charging;
                }
                if (devSoftwareVersionCmp(device->version(), ui->infoLabel->text())) {
                    upgradeRequirements |= UpgradeRequirements::LowerVersion;
                }
                item->setData(Qt::UserRole, upgradeRequirements);
                item->setText(upgradeRequirementsDescription(upgradeRequirements));
            }
                break;
            case DeviceFileTransferProgress: {
                item->setData(Qt::UserRole, 0);
                item->setText("0");
            }
                break;
            case DeviceUpgradePhase: {
                item->setText(upgradePhaseDescription(UpgradePhase::Wait));
            }
                break;
            case DeviceUpgradeResult: {
                item->setText(upgradeResultDescription(UpgradeResult::Unfinished));
            }
                break;
            case DeviceRetryButton: {
                item->setText(tr("retry"));
            }
                break;
            default: {
                break;
            }
            }
        }
        connect(device, &CommonDeviceItem::chargingStateChanged, this, &DeviceUpgradeDlg::onDevChargingStateChanged);
        connect(device, &CommonDeviceItem::versionChanged, this, &DeviceUpgradeDlg::onDevVersionChanged);
        connect(device, &CommonDeviceItem::cmdConnectionStateChanged, this, &DeviceUpgradeDlg::onDevCmdConnectionStateChanged);
        connect(device, &CommonDeviceItem::transferConnectionStateChanged, this, &DeviceUpgradeDlg::onDevTransferConnectionStateChanged);
        connect(device, &CommonDeviceItem::fileTransferProgressed, this, &DeviceUpgradeDlg::onDevFileTransferProgressed);
        connect(device, &CommonDeviceItem::upgradeResultBroadcast, this, &DeviceUpgradeDlg::onUpgradeResultBroadcast);
        connect(ui->deviceTableWidget, &QTableWidget::cellClicked, this, &DeviceUpgradeDlg::onDeviceTableCellClicked);
    }
}

void DeviceUpgradeDlg::updateDeviceTable()
{
    for (int row = 0; row < devices_.count(); ++row) {
        CommonDeviceItem *device = devices_[row];
        for (int col = DeviceName; col < Count; ++col) {
            QTableWidgetItem *item = ui->deviceTableWidget->item(row, col);
            switch(col) {
            case DeviceName: {
                item->setText(device->deviceName());
            }
                break;
            case DeviceSn: {
                item->setText(device->sn);
            }
                break;
            case DeviceVersion: {
                item->setText(device->version());
            }
                break;
            case DeviceUpgradeRequirements: {
                int upgradeRequirements = 0;
                if (device->cmdSocketConnected()) {
                    upgradeRequirements |= UpgradeRequirements::CmdConnected;
                }
                if (device->charging()) {
                    upgradeRequirements |= UpgradeRequirements::Charging;
                }
                if (devSoftwareVersionCmp(device->version(), ui->infoLabel->text())) {
                    upgradeRequirements |= UpgradeRequirements::LowerVersion;
                }
                item->setData(Qt::UserRole, upgradeRequirements);
                item->setText(upgradeRequirementsDescription(upgradeRequirements));
            }
                break;
            case DeviceFileTransferProgress: {
                item->setData(Qt::UserRole, 0);
                item->setText(QString::number(0));
            }
                break;
            default: {
                break;
            }
            }
        }
    }
}

void DeviceUpgradeDlg::updateDeviceTable(CommonDeviceItem *device)
{
    int row = devices_.indexOf(device);
    for (int col = DeviceName; col < Count; ++col) {
        QTableWidgetItem *item = ui->deviceTableWidget->item(row, col);
        switch(col) {
        case DeviceName: {
            item->setText(device->deviceName());
        }
            break;
        case DeviceSn: {
            item->setText(device->sn);
        }
            break;
        case DeviceVersion: {
            item->setText(device->version());
        }
            break;
        case DeviceUpgradeRequirements: {
            int upgradeRequirements = 0;
            if (device->cmdSocketConnected()) {
                upgradeRequirements |= UpgradeRequirements::CmdConnected;
            }
            if (device->charging()) {
                upgradeRequirements |= UpgradeRequirements::Charging;
            }
            if (devSoftwareVersionCmp(device->version(), ui->infoLabel->text())) {
                upgradeRequirements |= UpgradeRequirements::LowerVersion;
            }
            item->setData(Qt::UserRole, upgradeRequirements);
            item->setText(upgradeRequirementsDescription(upgradeRequirements));
        }
            break;
        case DeviceFileTransferProgress: {
            item->setData(Qt::UserRole, 0);
        }
            break;
        default: {
            break;
        }
        }
    }
}

void DeviceUpgradeDlg::updateDeviceTable(CommonDeviceItem *device, DeviceUpgradeDlg::ColumnContent content)
{
    int row = devices_.indexOf(device);
    QTableWidgetItem *item = ui->deviceTableWidget->item(row, content);
    switch(content) {
    case DeviceName: {
        item->setText(device->deviceName());
    }
        break;
    case DeviceSn: {
        item->setText(device->sn);
    }
        break;
    case DeviceVersion: {
        item->setText(device->version());
    }
        break;
    case DeviceUpgradeRequirements: {
        int upgradeRequirements = 0;
        if (device->cmdSocketConnected()) {
            upgradeRequirements |= UpgradeRequirements::CmdConnected;
        }
        if (device->charging()) {
            upgradeRequirements |= UpgradeRequirements::Charging;
        }
        if (devSoftwareVersionCmp(device->version(), ui->infoLabel->text())) {
            upgradeRequirements |= UpgradeRequirements::LowerVersion;
        }
        item->setData(Qt::UserRole, upgradeRequirements);
        item->setText(upgradeRequirementsDescription(upgradeRequirements));
    }
        break;
    case DeviceFileTransferProgress: {
        item->setData(Qt::UserRole, device->fileTransferProgress());
        item->setText(QString::number(device->fileTransferProgress()));
    }
        break;
    default: {
        break;
    }
    }
}

void DeviceUpgradeDlg::onDeviceTableCellClicked(int row, int column)
{
    if (column == DeviceRetryButton && upgradeThread_) {
        upgradeThread_->retry(devices_[row]);
    }
}

void DeviceUpgradeDlg::onImportClicked()
{
    QString upgradePkgPath = QFileDialog::getOpenFileName(this, tr("import upgrade package"), Config::upgradeFolderAbsPath(), tr("Images (*.tar)"));
    if (upgradePkgPath.isEmpty()) {
        return;
    }
    packageImportThread_ = new PackageImportThread(upgradePkgPath);
    connect(packageImportThread_, &PackageImportThread::finished, this, &DeviceUpgradeDlg::onPackageImportThreadFinished);
    connect(packageImportThread_, &PackageImportThread::importStart, this, &DeviceUpgradeDlg::onPackageImportStart);
    connect(packageImportThread_, &PackageImportThread::importProcessing, this, &DeviceUpgradeDlg::onPackageImportProcessing);
    connect(packageImportThread_, &PackageImportThread::importFinished, this, &DeviceUpgradeDlg::onPackageImportFinished);
    connect(packageImportThread_, &PackageImportThread::importCanceled, this, &DeviceUpgradeDlg::onPackageImportCanceled);
    packageImportThread_->start();
}

void DeviceUpgradeDlg::onStopImportClicked()
{
    if (packageImportThread_) {
        packageImportThread_->stopImport();
    }
    ui->stopImportPushButton->hide();
    ui->upgradePushButton->show();
    ui->importProgressBar->hide();
    ui->infoLabel->hide();
}

void DeviceUpgradeDlg::onUpgradeClicked()
{
    warningwidget *warning = new warningwidget(QString::fromLocal8Bit("为保证设备安全升级必须连接电源，升级过程中请不要关机和断开电源。"));
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
    dialog->setDisplayWidget(warning);
    dialog->setOkBtnText(QString::fromLocal8Bit("确认"));
    dialog->setCancelBtnText(QString::fromLocal8Bit("取消"));
    connect(dialog, &CommonDialog::sigAccepted, this, &DeviceUpgradeDlg::saveCheck);
    dialog->show();
}


void DeviceUpgradeDlg::saveCheck(bool ok)
{
    if(ok)
    {
        SaveUserDataConfirmDlg *saveUserDataConfirmDlg = new SaveUserDataConfirmDlg(tr("are you sure to upgrade?"), this);
        connect(saveUserDataConfirmDlg, &SaveUserDataConfirmDlg::sigAccepted, this, &DeviceUpgradeDlg::startUpgradeProcess);
        CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, this);
        dialog->setDisplayWidget(saveUserDataConfirmDlg);
        connect(saveUserDataConfirmDlg, &SaveUserDataConfirmDlg::sigAccepted, dialog, &CommonDialog::close);
        dialog->setMinimumSize(580,230);
        dialog->setMaximumSize(580,230);
        dialog->show();
    }
}

void DeviceUpgradeDlg::onStopUpgradeClicked()
{
    warningwidget *warning = new warningwidget(QString::fromLocal8Bit("正在批量升级中，是否终止升级？"));
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
    dialog->setDisplayWidget(warning);
    dialog->setOkBtnText(QString::fromLocal8Bit("确认"));
    dialog->setCancelBtnText(QString::fromLocal8Bit("取消"));
//    connect(dialog, &CommonDialog::sigAccepted, this, );
    dialog->show();
//    if (upgradeThread_) {
//        upgradeThread_->pauseUpgrade();
//    }
}

void DeviceUpgradeDlg::onPackageImportThreadFinished()
{
    if (packageImportThread_) {
        packageImportThread_->deleteLater();
        packageImportThread_ = nullptr;
    }
}

void DeviceUpgradeDlg::onPackageImportStart()
{
    ui->infoLabel->hide();
    ui->stopImportPushButton->show();
    ui->importProgressBar->setValue(0);
    ui->importProgressBar->show();
}

void DeviceUpgradeDlg::onPackageImportProcessing(int val)
{
    ui->importProgressBar->setValue(val);
}

void DeviceUpgradeDlg::onPackageImportFinished(bool success, QString errorStr)
{
    ui->stopImportPushButton->hide();
    ui->importProgressBar->hide();
    readUpgradePackageInfo();
    updateDeviceTable();
    if (success) {
        QMessageBox::information(this, tr("import upgrade package"), tr("import finished"));
    } else {
        QMessageBox::warning(this, tr("import upgrade package"), errorStr);
    }
}

void DeviceUpgradeDlg::onPackageImportCanceled()
{
    readUpgradePackageInfo();
}

void DeviceUpgradeDlg::onUpgradeThreadFinished()
{
    if (upgradeThread_) {
        upgradeThread_->deleteLater();
        upgradeThread_ = nullptr;
    }
}

void DeviceUpgradeDlg::onDevCmdConnectionStateChanged(bool /*newState*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        updateDeviceTable(dev);
    }
}

void DeviceUpgradeDlg::onDevTransferConnectionStateChanged(bool /*newState*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        updateDeviceTable(dev);
    }
}

void DeviceUpgradeDlg::onDevVersionChanged(QString oldVersion, QString newVersion)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        updateDeviceTable(dev);
    }
}

void DeviceUpgradeDlg::onDevChargingStateChanged(bool /*newStatus*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        updateDeviceTable(dev);
    }
}

void DeviceUpgradeDlg::onDevFileTransferProgressed(qint64 /*value*/)
{
    if (auto dev = dynamic_cast<CommonDeviceItem*>(sender())) {
        updateDeviceTable(dev, DeviceFileTransferProgress);
    }
}

void DeviceUpgradeDlg::onUpgradeResultBroadcast(bool /*res*/)
{

}

void DeviceUpgradeDlg::onUpgradeFinished(QList<int> upgradeRes)
{
    for (int row = 0; row < upgradeRes.count(); ++row) {
        QTableWidgetItem *item = ui->deviceTableWidget->item(row, DeviceUpgradeResult);
        item->setText(upgradeResultDescription(static_cast<UpgradeResult>(upgradeRes.at(row))));
    }

    QStringList upgradeInfo;
    int succeedCount = 0;
    for (int i = 0; i < upgradeRes.count(); ++i) {
        QString deviceUpgradeInfo = QString("%1 - %2")
                .arg(ui->deviceTableWidget->item(i, DeviceName)->text())
                .arg(ui->deviceTableWidget->item(i, DeviceUpgradeResult)->text());
        if (upgradeRes.at(i) == UpgradeResult::Succeed) {
            ++succeedCount;
        }
        upgradeInfo.append(deviceUpgradeInfo);
    }
    upgradeInfo.append(QString("%1 succeed (total: %2)").arg(succeedCount).arg(upgradeRes.count()));
    QMessageBox::information(this, tr("upgrade result"), upgradeInfo.join("\n"));
}

void DeviceUpgradeDlg::onStartUpgradeProgcessRequest(CommonDeviceItem *device, QString imgName, QString version, bool saveUserData)
{
    if (devices_.contains(device)) {
        device->onStartUpgradeProgcessRequest(imgName, version, saveUserData);
    }
}

void DeviceUpgradeDlg::onUpgradePhaseChanged(CommonDeviceItem *device, int upgradePhase)
{
    int row = devices_.indexOf(device);
    LOG(INFO) << "row=" << row << " upgrade phase=" << upgradePhase;
    QTableWidgetItem *item = ui->deviceTableWidget->item(row, DeviceUpgradePhase);
    item->setText(upgradePhaseDescription(upgradePhase));
    if (upgradePhase != UpgradePhase::TransferImage) {
        item = ui->deviceTableWidget->item(row, DeviceFileTransferProgress);
        item->setData(Qt::UserRole, 0);
        item->setText("0");
    }
}

void DeviceUpgradeDlg::readUpgradePackageInfo()
{
    ui->upgradeImgLineEdit->setText(tr("import upgrade package"));
    ui->infoLabel->hide();
    ui->upgradePushButton->setEnabled(true);

    QString upgradeFolderPath = Config::upgradeFolderAbsPath();
    QDir upgradeFolder(upgradeFolderPath);
    QString packageName;
    QString version;
    QString md5;
    if (upgradeFolder.exists()) {
        bool readSuc = false;
        for (auto fileInfo : upgradeFolder.entryInfoList(QDir::Files)) {
            if (fileInfo.fileName().endsWith("config.ini")) {
                QSettings config(fileInfo.absoluteFilePath(), QSettings::IniFormat);
                version = config.value("Config/version").toString();
                if (version.isEmpty()) {
                    break;
                }
                md5 = config.value("Config/MD5").toString();
                if (md5.isEmpty()) {
                    break;
                }
                QString nameInConfig = config.value("Config/name").toString();
                if (!nameInConfig.isEmpty()) {
                    packageName = nameInConfig;
                }
            } else {
                packageName = fileInfo.fileName();
            }
            if (!version.isEmpty() && !md5.isEmpty() && !packageName.isEmpty()) {
                readSuc =  true;
                break;
            }
        }
        if (readSuc) {
            ui->infoLabel->setText(version);
            ui->upgradeImgLineEdit->setText(packageName);

            ui->infoLabel->show();
            ui->upgradePushButton->setEnabled(true);
        } else {
            upgradeFolder.removeRecursively();
        }
    }
}

bool DeviceUpgradeDlg::devSoftwareVersionCmp(QString deviceVersion, QString imgVersion)
{
    if (deviceVersion.split("_").count() != 2) {
        return false;
    }
//    if (ver1.split("_").count() != 2) {
//        return false;
//    }
    QStringList deviceVersionLst = deviceVersion.split("_")[1].split(".");
    QStringList imgVersionLst = imgVersion.split(".");
    for (int i = 0; i < qMin(deviceVersionLst.count(), imgVersionLst.count()); ++i) {
        if (deviceVersionLst[i].toInt() < imgVersionLst[i].toInt()) {
            return true;
        } else if (deviceVersionLst[i].toInt() > imgVersionLst[i].toInt()) {
            return false;
        }
    }
    return false; // return deviceVersion < imgVersion
}

QString DeviceUpgradeDlg::upgradeRequirementsDescription(int requirements)
{
    if (!(requirements & CmdConnected)) {
        return tr("disconnected");
    } else if (!(requirements & Charging)) {
        return tr("not charging");
    } else if (!(requirements & LowerVersion)) {
        return tr("latest version");
    } else {
        return (tr("connected"));
    }
}

QString DeviceUpgradeDlg::upgradePhaseDescription(int upgradePhase)
{
    switch(upgradePhase) {
    case UpgradePhase::Wait: {
        return tr("wait");
    }
    case UpgradePhase::TransferImage: {
        return tr("transferImage");
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

QString DeviceUpgradeDlg::upgradeResultDescription(int upgradeResult)
{
    switch(upgradeResult) {
    case UpgradeResult::Unfinished: {
        return tr("unfinished");
    }
    case UpgradeResult::Succeed: {
        return tr("succeed");
    }
    case UpgradeResult::Fail: {
        return tr("fail");
    }
    case UpgradeResult::Timeout: {
        return tr("timeout");
    }
    default:
        return tr("unknown");
    }
}

void DeviceUpgradeDlg::startUpgradeProcess(bool accepted, bool saveUserData)
{
    if (accepted) {
        QDir upgradeFolder(Config::upgradeFolderAbsPath());
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
        for (int i = 0; i < devices_.count(); ++i) {
            if (auto tableItem = ui->deviceTableWidget->item(i, DeviceUpgradeRequirements)) {
                auto upgradeRequirements = tableItem->data(Qt::UserRole).toInt();
                if (upgradeRequirements == UpgradeRequirements::Upgradable) {
                    devicesToUpgrade.append(devices_[i]);
                }
            }
        }
        if (devicesToUpgrade.count() == 0) {
            QMessageBox::information(this, tr("upgrade"), tr("no device need to upgrade"));
            return;
        }
        if (imageName.isEmpty()) {
            QMessageBox::information(this, tr("upgrade"), tr("no valid upgrade image found"));
            return;
        }

        if (upgradeThread_) {
            upgradeThread_->retryUpgrade(devicesToUpgrade);
        } else {
            qRegisterMetaType<QList<int>>("QList<int>");
            upgradeThread_ = new DeviceUpgradeThread(devicesToUpgrade, imageName, version, saveUserData);
            connect(upgradeThread_, &DeviceUpgradeThread::finished, this, &DeviceUpgradeDlg::onUpgradeThreadFinished);
            connect(upgradeThread_, &DeviceUpgradeThread::upgradeFinished, this, &DeviceUpgradeDlg::onUpgradeFinished);
            connect(upgradeThread_, &DeviceUpgradeThread::requestStartUpgradeProgcess, this, &DeviceUpgradeDlg::onStartUpgradeProgcessRequest);
            connect(upgradeThread_, &DeviceUpgradeThread::upgradePhaseChanged, this, &DeviceUpgradeDlg::onUpgradePhaseChanged);
            upgradeThread_->start();
        }
        ui->stopUpgradePushButton->show();
        ui->upgradePushButton->hide();
        ui->importPushButton->setEnabled(false);
    }
}

