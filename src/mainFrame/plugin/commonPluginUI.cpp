#include "commonPluginUI.h"

#include "logHelper.h"
#include "settings.h"
#include "md5.h"
#include "fileDirHandler.h"

#include "commonClassroom.h"
#include "commonDeviceItem.h"
#include "devicemanagement.pb.h"
#include "communicator.h"
#include "adddeviceswidget.h"
#include "commondialog.h"
#include "fileShareWidget.h"
#include "deviceUpgradeWidget.h"
#include "warningwidget.h"

#include <QTcpSocket>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QString>
#include <QStringList>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QResizeEvent>
#include <QFile>
#include <QTabWidget>

#include <QDebug>

CommonPluginUI::CommonPluginUI(const DevPluginInterface *devPlugin, QWidget *parent)
    : DevPluginUI(parent)
    , devPlugin_(devPlugin)
    , activative_(false)
{
    QFile styleSheet(":/res/qss/plugin.qss");
    if(styleSheet.open(QIODevice::ReadOnly))
    {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    auto mainLayout = new QHBoxLayout(this);
    auto margins = mainLayout->contentsMargins();
    margins.setLeft(20);
    mainLayout->setContentsMargins(margins);
    auto cmdsWidget = new QWidget;
    cmdsWidget->setObjectName("cmdsWidget");
    cmdsWidget->setFixedWidth(170);
    mainLayout->addWidget(cmdsWidget);
    cmdsLayout_ = new QVBoxLayout(cmdsWidget);
    margins = cmdsLayout_->contentsMargins();
    margins.setTop(20);
    margins.setBottom(20);
    cmdsLayout_->setContentsMargins(margins);

    QString toolTipText(tr("Equipment can be selected in batches, which is more efficient!"));

    shutdownBtn_ = new QPushButton(tr("shutdown"));
    shutdownBtn_->setObjectName("shutdownBtn");
    shutdownBtn_->setIcon(QIcon(":/res/images/ic_turn_off_disable.svg"));
    shutdownBtn_->setIconSize(QSize(24,24));
    shutdownBtn_->setToolTip(toolTipText);
    connect(shutdownBtn_, &QPushButton::clicked, this, &CommonPluginUI::onShutdownClicked);
    cmdsLayout_->addWidget(shutdownBtn_);

    rebootBtn_ = new QPushButton(tr("reboot"));
    rebootBtn_->setObjectName("rebootBtn");
    rebootBtn_->setIcon(QIcon(":/res/images/ic_restart_disable.svg"));
    rebootBtn_->setIconSize(QSize(24,24));
    rebootBtn_->setToolTip(toolTipText);
    connect(rebootBtn_, &QPushButton::clicked, this, &CommonPluginUI::onRebootClicked);
    cmdsLayout_->addWidget(rebootBtn_);

    resetBtn_ = new QPushButton(tr("reset"));
    resetBtn_->setObjectName("resetBtn");
    resetBtn_->setIcon(QIcon(":/res/images/ic_restore_factory_disable.svg"));
    resetBtn_->setIconSize(QSize(24,24));
    resetBtn_->setToolTip(toolTipText);
    connect(resetBtn_, &QPushButton::clicked, this, &CommonPluginUI::onResetClicked);
    cmdsLayout_->addWidget(resetBtn_);

    upgradeBtn_ = new QPushButton(tr("upgrade"));
    upgradeBtn_->setObjectName("upgradeBtn");
    upgradeBtn_->setIcon(QIcon(":/res/images/ic_update_disable.svg"));
    upgradeBtn_->setIconSize(QSize(24,24));
    upgradeBtn_->setToolTip(toolTipText);
    cmdsLayout_->addWidget(upgradeBtn_);
    connect(upgradeBtn_, &QPushButton::clicked, this, &CommonPluginUI::onUpgradeClicked);

    lockBtn_ = new QPushButton(tr("lock devices"));
    lockBtn_->setObjectName("lockBtn");
    lockBtn_->setIcon(QIcon(":/res/images/ic_lock_device_disable.svg"));
    lockBtn_->setIconSize(QSize(24,24));
    lockBtn_->setToolTip(toolTipText);
    connect(lockBtn_, &QPushButton::clicked, this, &CommonPluginUI::onLockClicked);
    cmdsLayout_->addWidget(lockBtn_);

    screenCastBtn_ = new QPushButton(tr("screen cast"));
    screenCastBtn_->setObjectName("screenCastBtn");
    screenCastBtn_->setIcon(QIcon(":/res/images/ic_screen_projection_disable.svg"));
    screenCastBtn_->setIconSize(QSize(24,24));
    screenCastBtn_->setToolTip(toolTipText);
    connect(screenCastBtn_, &QPushButton::clicked, this, &CommonPluginUI::onScreenCastClicked);
    cmdsLayout_->addWidget(screenCastBtn_);

    screenMonitorBtn_ = new QPushButton(tr("screen monitor"));
    screenMonitorBtn_->setObjectName("screenMonitorBtn");
    screenMonitorBtn_->setIcon(QIcon(":/res/images/ic_screen_monitor_disable.svg"));
    screenMonitorBtn_->setIconSize(QSize(24,24));
    screenMonitorBtn_->setToolTip(toolTipText);
    connect(screenMonitorBtn_, &QPushButton::clicked, this, &CommonPluginUI::onScreenMonitorClicked);
    cmdsLayout_->addWidget(screenMonitorBtn_);

//    btn = new QPushButton(tr("broadcast"));
//    cmdsLayout_->addWidget(btn);
//    connect(btn, &QPushButton::clicked, this, &CommonPluginUI::onBroadcastClicked);

//    btn = new QPushButton(tr("stop broadcast"));
//    cmdsLayout_->addWidget(btn);
//    connect(btn, &QPushButton::clicked, this, [this]() {
//        communicator_->stopBroadcast();
    //    });


    cmdsLayout_->addStretch(1);

    auto btn = new QPushButton(tr("share files"));
    btn->setObjectName("shareFilesBtn");
    btn->setIcon(QIcon(":/res/images/ic_share.svg"));
    btn->setIconSize(QSize(24,24));
    cmdsLayout_->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, &CommonPluginUI::onShareFileClicked);

    btn = new QPushButton(tr("add device"));
    btn->setObjectName("addDeviceBtn");
    btn->setIcon(QIcon(":/res/images/ic_add_device.svg"));
    btn->setIconSize(QSize(24,24));
    cmdsLayout_->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, static_cast<void (CommonPluginUI::*)()>(&CommonPluginUI::onAddDeviceClicked));
    addDevice2_ = btn;

    btn = new QPushButton(tr("network"));
    btn->setObjectName("networkBtn");
    btn->setIcon(QIcon(":/res/images/ic_distribution_network.svg"));
    btn->setIconSize(QSize(24,24));
    cmdsLayout_->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, &CommonPluginUI::onNetworkClicked);

    classroom_ = new CommonClassroom(devPlugin);
    connect(classroom_, &CommonClassroom::deviceDeleted,
            this,&CommonPluginUI::checkDeviceNum);
    connect(classroom_, &CommonClassroom::addBtnClicked,
            this,&CommonPluginUI::onAddDeviceClicked);
    connect(classroom_,&CommonClassroom::selectionChanged,this,&CommonPluginUI::onSelectionChanged);
    mainLayout->addWidget(classroom_);

    checkDeviceNum();

    communicator_ = Communicator::instance();
    onBroadcastClicked();
    initActivateCover();
    deactivate();
}

CommonPluginUI::~CommonPluginUI()
{
    communicator_ = nullptr;
    classroom_ = nullptr;
    cmdsLayout_ = nullptr;
    addDevice2_ = nullptr;
    lockBtn_ = nullptr;
    shutdownBtn_ = nullptr;
    rebootBtn_ = nullptr;
    resetBtn_ = nullptr;
    upgradeBtn_ = nullptr;
    screenCastBtn_ = nullptr;
    screenMonitorBtn_ = nullptr;
    activateCover_ = nullptr;
    activateBtn_ = nullptr;
}

void CommonPluginUI::load(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        QByteArray classroomData = file.readAll();
        file.close();
        auto jsonDocument = QJsonDocument::fromJson(classroomData);
        auto autoSorting = jsonDocument[ClassroomFileKey::AutomaticSorting];
        bool bAutoSorting = false;
        if(autoSorting.type() != QJsonValue::Undefined) {
            bAutoSorting = autoSorting.toBool();
        }
        auto deviceTypeNameInFile = jsonDocument[ClassroomFileKey::DeviceTypeName].toString();
        if (!deviceTypeNameInFile.isEmpty() && (deviceTypeNameInFile == devPlugin_->name())) {
            classroom_->load(jsonDocument.object());
            classroom_->setPositionLocked(bAutoSorting);
        } else {
            LOG(INFO) << "device type not match, device of current console is " << devPlugin_->name().toStdString() << " and in file is " << deviceTypeNameInFile.toStdString();
            return;
        }
    }

    checkDeviceNum();
}

QString CommonPluginUI::save(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QFile::ReadWrite | QFile::Truncate)) {
        QJsonObject jsonObj;
        jsonObj[ClassroomFileKey::DeviceTypeName] = devPlugin_->name();
        jsonObj[ClassroomFileKey::AutomaticSorting] = classroom_->positionLocked();
        classroom_->save(jsonObj);
        QJsonDocument jsonDocument;
        jsonDocument.setObject(jsonObj);
        file.write(jsonDocument.toJson());
        file.close();
        return filePath;
    }
    return "";
}

void CommonPluginUI::activate()
{
    activative_ = true;
    activateCover_->setVisible(false);
    activateBtn_->setVisible(false);
    connect(communicator_, &Communicator::broadcastResponsed, this, &CommonPluginUI::onBroadcastResponsed);
    connect(communicator_, &Communicator::newCmdConnection, this, &CommonPluginUI::onNewCmdConnection);
    connect(communicator_, &Communicator::newTransferConnection, this, &CommonPluginUI::onNewTransferConnection);
    connect(communicator_, &Communicator::broadcastSignal, this, &CommonPluginUI::onBroadcastSignal);
}

void CommonPluginUI::deactivate()
{
    activative_ = false;
    activateCover_->setVisible(true);
    activateCover_->raise();
    activateBtn_->setVisible(true);
    activateBtn_->raise();
    disconnect(communicator_, &Communicator::broadcastResponsed, this, &CommonPluginUI::onBroadcastResponsed);
    disconnect(communicator_, &Communicator::newCmdConnection, this, &CommonPluginUI::onNewCmdConnection);
    disconnect(communicator_, &Communicator::newTransferConnection, this, &CommonPluginUI::onNewTransferConnection);
}


void CommonPluginUI::onShutdownClicked()
{
//    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
//    QString warningStr = tr("Are you sure you need to perform a batch shutdown operation?\n\n"
//                            "Note: Please make sure that the robot has been placed in a safe location,\n"
//                            "avoid damage to the robot from falling down after power off.");
//    warningwidget *warning = new warningwidget(warningStr,dialog);
//    dialog->setDisplayWidget(warning);
//    dialog->setMinimumSize(580,130);
//    connect(dialog, &CommonDialog::sigAccepted, this, &CommonPluginUI::Shutdown);
//    dialog->show();

    auto shutDownProcedure = devPlugin_->createShutDownProcedure(this);
    connect(shutDownProcedure, &ConfirmProcedure::confirmComplete, [this](bool accepted, int mode){
        Shutdown(accepted);
    });
    shutDownProcedure->startProcedure();
}

void CommonPluginUI::Shutdown(bool ok)
{
    if (ok) {
        setBtnsEnabled(false);
        classroom_->shutdownDevices();
    }
}

void CommonPluginUI::onRebootClicked()
{

    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
    warningwidget *warning = new warningwidget(QString::fromLocal8Bit("是否确定要执行批量重启操作?重启后设备将自动重新连接。"),dialog);
    dialog->setDisplayWidget(warning);
    dialog->setMinimumSize(580,130);
    connect(dialog, &CommonDialog::sigAccepted, this, &CommonPluginUI::Reboot);
    dialog->show();
}

void CommonPluginUI::Reboot(bool ok)
{
    if(ok)
    {
        setBtnsEnabled(false);
        classroom_->rebootDevices();
    }
}

void CommonPluginUI::onResetClicked()
{
//    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
//    warningwidget *warning = new warningwidget(QString::fromLocal8Bit("为保证设备安全恢复出厂必须连接电源，恢复过程中请不要关机和断开电源。"),dialog);
//    dialog->setDisplayWidget(warning);
//    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted){
//        if(accepted) {
//            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, this);
//            SaveUserDataWidget *saveUserDataWidget = new SaveUserDataWidget(QString::fromLocal8Bit("恢复后设备将会失去之前保留在设备上的数据，如果需要保留数据，请把数据备份到userdata目录下，并勾选保留userdata选项。"), dialog);
//            connect(saveUserDataWidget, &SaveUserDataWidget::sigAccepted, classroom_, &CommonClassroom::resetDevices);
//            dialog->setDisplayWidget(saveUserDataWidget);
//            connect(saveUserDataWidget, &SaveUserDataWidget::sigAccepted, dialog, &CommonDialog::close);
//            dialog->setFixedSize(580,230);
//            dialog->show();
//        }
//    });
//    dialog->show();

    auto upgradeConfirmProcedure = devPlugin_->createResetConfirmProcedure(this);
    connect(upgradeConfirmProcedure, &ConfirmProcedure::confirmComplete, classroom_, &CommonClassroom::resetDevices);
    upgradeConfirmProcedure->startProcedure();
}

void CommonPluginUI::onUpgradeClicked()
{
    CommonDialog *dialog = new CommonDialog(tr("upgrade"), CommonDialog::NoButton, this);
    DeviceUpgradeWidget* deviceUpgradeWidget = new DeviceUpgradeWidget(devPlugin_, classroom_->selectedDeviceItems().values(), dialog);
    dialog->setDisplayWidget(deviceUpgradeWidget);
    dialog->setCloseOnClose(false);
    connect(deviceUpgradeWidget, &DeviceUpgradeWidget::closeRequest, dialog, &CommonDialog::close);
    connect(dialog,&CommonDialog::sigClosed,deviceUpgradeWidget,&DeviceUpgradeWidget::onClose);
    dialog->show();
}

void CommonPluginUI::onLockClicked()
{
    bool allLocked = classroom_->selectedDevicesAllLocked();
    bool targetLockState = !allLocked;
    if (targetLockState) {
        lockBtn_->setText(tr("unlock devices"));
        lockBtn_->setIcon(QIcon(":/res/images/ic_unlock_device.svg"));

    } else {
        lockBtn_->setText(tr("lock devices"));
        lockBtn_->setIcon(QIcon(":/res/images/ic_lock_device.svg"));
    }
//    for (auto &selectedDevice : classroom_->selectedDeviceItems()) {
//        if (selectedDevice->locked() == targetLockState) {
//            continue;
//        }
//        selectedDevice->setLocked(targetLockState);
//        if (selectedDevice->deviceConnected()) {
//            selectedDevice->update();
//            dm::CmdPackage cmdPkg;
//            cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_LOCK);
//            cmdPkg.set_lock(targetLockState);
//            dm::TransMsg transPkg;
//            transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
//            transPkg.set_packagecontent(cmdPkg.SerializeAsString());
//            auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
//            selectedDevice->cmdSocket->write(data);
//            selectedDevice->cmdSocket->flush();
//        }
//    }
    setBtnsEnabled(false);
    classroom_->lockDevices(targetLockState);
}

void CommonPluginUI::createDialog()
{
    CommonDialog* dialog = new CommonDialog(tr("System Prompt"),CommonDialog::OnlyOkButton,this);
    dialog->setDisplayText(tr("Coming soon..."));
    dialog->show();
}

void CommonPluginUI::onScreenCastClicked()
{
    createDialog();
}

void CommonPluginUI::onScreenMonitorClicked()
{
    createDialog();
}

void CommonPluginUI::onShareFileClicked()
{
    CommonDialog *dialog = new CommonDialog(tr("share files"), CommonDialog::NoButton, this);
    FileShareWidget *fileShareWidget = new FileShareWidget;
    connect(fileShareWidget,&FileShareWidget::shareListChanged,this,&CommonPluginUI::onShareListChanged);
    dialog->setDisplayWidget(fileShareWidget);
    dialog->onSetOkBtnEnabled(false);
    dialog->exec();
}

void CommonPluginUI::onAddDeviceClicked()
{
//    auto sn = QInputDialog::getText(nullptr, tr("add device"), tr("device sn:"));
//    QMap<QString, QString> sn2id;
//    sn2id.insert(sn, sn);
//    addDevices(sn2id);
    CommonDialog* dialog = new CommonDialog(tr("Add Devices"),CommonDialog::OkCancelButton,this);
    int currentDeviceNum = classroom_->deviceItems().size();
    AddDevicesWidget* widget = new AddDevicesWidget(kMaxDeviceNum-currentDeviceNum,dialog);
    connect(widget, &AddDevicesWidget::addDevices, this, &CommonPluginUI::addDevices);
    connect(widget, &AddDevicesWidget::setOkBtnEnabled, dialog, &CommonDialog::onSetOkBtnEnabled);
    connect(widget, &AddDevicesWidget::setFocusToOkBtn, dialog, &CommonDialog::onSetFocusToOkBtn);
    connect(dialog, &CommonDialog::sigAccepted, widget, &AddDevicesWidget::onAccepted);
    dialog->setDisplayWidget(widget);
    dialog->onSetOkBtnEnabled(false);
    dialog->show();
}

void CommonPluginUI::checkDeviceNum()
{
    int deviceNum = classroom_->deviceItems().size();
    if (deviceNum == kMaxDeviceNum) {
        addDevice2_->setEnabled(false);
    } else {
        addDevice2_->setEnabled(true);
    }
    if (deviceNum == 0) {
        setBtnsEnabled(false);
    }
}

void CommonPluginUI::addDevices(const QList<QPair<QString, QString>> &sns2ids)
{
    if (auto classroomContainer = container()) {
        QSet<QString> newDevicesSNs;
        for (auto sn2id : sns2ids) {
            newDevicesSNs.insert(sn2id.first);
        }
        QStringList errorStr;
        for (int i = 0; i < classroomContainer->count(); ++i) {
            if (auto openedClassroomWidget = dynamic_cast<DevPluginUI*>(classroomContainer->widget(i))) {
                auto intersectDevices = openedClassroomWidget->allDevicesSNs().intersect(newDevicesSNs);
                if (intersectDevices.count() != 0) {
                    for (auto intersectDevice : intersectDevices) {
                        errorStr.append(tr("device %1 has been added to %2 already").arg(intersectDevice).arg(classroomContainer->tabText(i)));
                    }
                }
            }
        }
        if (errorStr.count() != 0) {
            errorStr.prepend(tr("add device failed, can not add device already exisited in opened classroom:"));
            LOG(INFO) << errorStr.join("\n").toStdString();
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OnlyOkButton, this);
            warningwidget *warning = new warningwidget(errorStr.join("\n"), dialog);
            dialog->setDisplayWidget(warning);
            dialog->exec();
            return;
        }
    }

    classroom_->addDevices(sns2ids);
    checkDeviceNum();
}

void CommonPluginUI::onNetworkClicked()
{
    createDialog();
}

void CommonPluginUI::onBroadcastResponsed(const QHostAddress &ip, quint16 port, const QByteArray &responseData)
{
    dm::TransMsg transMsg;
    transMsg.ParseFromString(responseData.toStdString());
//    LOG(INFO) << "package type:" << transMsg.packagetype();
    if (dm::TransMsg_PACKAGETYPE_STATEPACKAGE == transMsg.packagetype()) { // 旧版本
        dm::StatePackage statePackage;
        statePackage.ParseFromString(transMsg.packagecontent());
        QString deviceSn = QString::fromStdString(statePackage.sn());
        auto devices = classroom_->deviceItems();
        LOG(INFO) << "device sn:" << deviceSn.toStdString() << " ip:" << ip.toString().toStdString();
        if (devices.contains(deviceSn)) {
            if (devPlugin_->deviceType() == 0) {
                devices[deviceSn]->setTypeMismatch(false);
                devices[deviceSn]->ips.insert(ip);
                devices[deviceSn]->resetLostBroadcastCount();
                if ((!devices[deviceSn]->cmdSocket || devices[deviceSn]->cmdSocket->state() == QAbstractSocket::UnconnectedState)) {
                    LOG(INFO) << "valid device sn for AiboxPlugin, and device is not connnected, ip is:" << ip.toString().toStdString();
                    dm::ConnectPackage connectPackage;
                    connectPackage.set_connect(true);
                    dm::TransMsg msgPkg;
                    msgPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CONNECTPACKAGE);
                    msgPkg.set_packagecontent(connectPackage.SerializeAsString());
                    auto data = QByteArray::fromStdString(msgPkg.SerializeAsString());
                    communicator_->udpMsg(ip, port, data);
                }
                QString version = statePackage.version().c_str();
                devices[deviceSn]->setVersion(version.trimmed());
                devices[deviceSn]->power = statePackage.power();
                devices[deviceSn]->setCharging(statePackage.charging());
                int statusCode = statePackage.state();
                LOG(INFO) << "statusCode:" << statusCode;
                if (devices[deviceSn]->isExecuting() &&
                        (devices[deviceSn]->cmd == dm::CmdPackage_CMDTYPE_RESET ||
                         devices[deviceSn]->cmd == dm::CmdPackage_CMDTYPE_UPGRADE) &&
                        statusCode != 0) {
                    devices[deviceSn]->onDeviceStatusChange(statusCode);
                }
                bool locked = statePackage.lock();
                devices[deviceSn]->onLockStatusResponse(locked);
            }
            else {
                devices[deviceSn]->setTypeMismatch(true);
            }

            devices[deviceSn]->update();
        }
    }
    else if (dm::TransMsg_PACKAGETYPE_STATUSPACKAGE == transMsg.packagetype()) {
        dm::StatusPackage statusPackage;
        statusPackage.ParseFromString(transMsg.packagecontent());
        QString deviceSn = QString::fromStdString(statusPackage.sn());
        int32_t deviceType = statusPackage.devicetype();
        auto devices = classroom_->deviceItems();
        LOG(INFO) << "device sn:" << deviceSn.toStdString()
                  << " device type:" << deviceType
                  << " ip:" << ip.toString().toStdString();
        if (devices.contains(deviceSn)) {
            if (devPlugin_->deviceType() == deviceType) {
                devices[deviceSn]->setTypeMismatch(false);
                devices[deviceSn]->ips.insert(ip);
                devices[deviceSn]->resetLostBroadcastCount();
                if ((!devices[deviceSn]->cmdSocket || devices[deviceSn]->cmdSocket->state() == QAbstractSocket::UnconnectedState)) {
                    LOG(INFO) << "valid device sn, and device is not connnected, ip is:" << ip.toString().toStdString();
                    dm::ConnectPackage connectPackage;
                    connectPackage.set_connect(true);
                    dm::TransMsg msgPkg;
                    msgPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CONNECTPACKAGE);
                    msgPkg.set_packagecontent(connectPackage.SerializeAsString());
                    auto data = QByteArray::fromStdString(msgPkg.SerializeAsString());
                    communicator_->udpMsg(ip, port, data);
                }
                QString version = statusPackage.version().c_str();
                devices[deviceSn]->setVersion(version.trimmed());
                devices[deviceSn]->power = statusPackage.power();
                devices[deviceSn]->setCharging(statusPackage.charging());
                int statusCode = statusPackage.state();
                LOG(INFO) << "statusCode:" << statusCode;
                if (devices[deviceSn]->isExecuting() &&
                        (devices[deviceSn]->cmd == dm::CmdPackage_CMDTYPE_RESET ||
                         devices[deviceSn]->cmd == dm::CmdPackage_CMDTYPE_UPGRADE) &&
                        statusCode != 0) {
                    devices[deviceSn]->onDeviceStatusChange(statusCode);
                }
                bool locked = statusPackage.lock();
                devices[deviceSn]->onLockStatusResponse(locked);
            } else {
                devices[deviceSn]->setTypeMismatch(true);
            }
            devices[deviceSn]->update();
        }
    }
    else if (dm::TransMsg_PACKAGETYPE_CONNECTRESPONSEPACKAGE == transMsg.packagetype()) {
        dm::ConnectResponsePackage connectRespPkg;
        connectRespPkg.ParseFromString(transMsg.packagecontent());
        QString deviceSn = QString::fromStdString(connectRespPkg.sn());
        QString serverIp = QString::fromStdString(connectRespPkg.serverip());
        auto devices = classroom_->deviceItems();
        LOG(INFO) << "connecting device sn:" << deviceSn.toStdString() << " server ip:" << serverIp.toStdString();
        if (devices.contains(deviceSn) && devices[deviceSn]->status == DeviceStatusCode::kNormalState) {
//            devices[deviceSn]->status = DeviceStatusCode::kAlreadyConnected;  // 不再提示异常
            devices[deviceSn]->connectedServerIp = serverIp;
        }
    }
//    else {
//        LOG(INFO) << "not dm::TransMsg_PACKAGETYPE_StatePackage, abandon";
//    }
}

void CommonPluginUI::onNewCmdConnection(QTcpSocket *tcpSocket)
{
    LOG(INFO) << "new connection ip:" << tcpSocket->peerAddress().toString().toStdString() << " port:" << tcpSocket->peerPort();
    for (auto &deviceItem : classroom_->deviceItems()) {
        if (deviceItem->ips.contains(tcpSocket->peerAddress())) {
            deviceItem->cmdSocket = tcpSocket;
            if (deviceItem->status == DeviceStatusCode::kAlreadyConnected) {
                deviceItem->status = DeviceStatusCode::kNormalState;
            }
            deviceItem->onCmdSocketConnect();
            if (deviceItem->cmd == dm::CmdPackage_CMDTYPE_REBOOT) {
                deviceItem->setStatusAndSignal(0);
            }
            //产品意见，连接后默认选中
            deviceItem->setSelectedAndSignal(true);
            deviceItem->update();
            connect(tcpSocket, &QTcpSocket::readyRead, deviceItem, &CommonDeviceItem::onCmdSocketReadyRead);
            connect(tcpSocket, &QTcpSocket::disconnected, deviceItem, &CommonDeviceItem::onCmdSocketDisconnect);
            return;
        }
    }

//    LOG(INFO) << "disconnect because device which ip is " << tcpSocket->peerAddress().toString().toStdString() << " is not in classroom";
//    tcpSocket->close();
//    tcpSocket->deleteLater();
}

void CommonPluginUI::onNewTransferConnection(QTcpSocket *tcpSocket)
{
    LOG(INFO) << "new transfer connection ip:" << tcpSocket->peerAddress().toString().toStdString() << " port:" << tcpSocket->peerPort();
    for (auto &deviceItem : classroom_->deviceItems()) {
        if (deviceItem->ips.contains(tcpSocket->peerAddress())) {
            deviceItem->transferSocket = tcpSocket;
            deviceItem->onTransferSocketConnect();
            deviceItem->update();
//            connect(tcpSocket, &QTcpSocket::disconnected, deviceItem, &CommonDeviceItem::onTransferSocketDisconnect);
            return;
        }
    }

//    LOG(INFO) << "disconnect because device which ip is " << tcpSocket->peerAddress().toString().toStdString() << " is not in classroom";
//    tcpSocket->close();
//    tcpSocket->deleteLater();
}

void CommonPluginUI::onSelectionChanged()
{
    if (classroom_) {
        if (classroom_->selectedDevicesAllLocked()) {
            if (lockBtn_) {
                lockBtn_->setText(tr("unlock devices"));
                lockBtn_->setIcon(QIcon(":/res/images/ic_unlock_device.svg"));
            }
        } else {
            if (lockBtn_) {
                lockBtn_->setText(tr("lock devices"));
                lockBtn_->setIcon(QIcon(":/res/images/ic_lock_device.svg"));
            }
        }

        int selectedSize = classroom_->selectedDeviceItems().size();
        if (selectedSize == 0) {
            setBtnsEnabled(false);
        } else {
            setBtnsEnabled(true);
        }
    }
}

void CommonPluginUI::onBroadcastClicked()
{
    dm::BroadcastPackage broadcastPkg;
    broadcastPkg.set_query("broadcast");
    dm::TransMsg transPkg;
    transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_BROADCASTPACKAGE);
    transPkg.set_packagecontent(broadcastPkg.SerializeAsString());
    auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
    communicator_->startBroadcast(data);
}

void CommonPluginUI::onStopBroadcastClicked()
{
    communicator_->stopBroadcast();
}

void CommonPluginUI::resizeEvent(QResizeEvent* e)
{
    activateCover_->setFixedSize(e->size().width(), e->size().height());
    activateBtn_->move(width() / 2 - 260 / 2, height() / 2 - 56 / 2);
    if (!activative_) {
        activateCover_->raise();
        activateBtn_->raise();
    }
    DevPluginUI::resizeEvent(e);
}

void CommonPluginUI::setBtnsEnabled(bool enabled)
{
    if (shutdownBtn_) {
        shutdownBtn_->setEnabled(enabled);
        if(enabled) {
            shutdownBtn_->setIcon(QIcon(":/res/images/ic_turn_off.svg"));
        } else {
            shutdownBtn_->setIcon(QIcon(":/res/images/ic_turn_off_disable.svg"));
        }
    }
    if (rebootBtn_) {
        rebootBtn_->setEnabled(enabled);
        if(enabled) {
            rebootBtn_->setIcon(QIcon(QPixmap(":/res/images/ic_restart2.png")));
        } else {
            rebootBtn_->setIcon(QIcon(":/res/images/ic_restart_disable.svg"));
        }
    }
    if (lockBtn_) {
        lockBtn_->setEnabled(enabled);
        if(enabled) {
            if(classroom_->selectedDevicesAllLocked()) {
                lockBtn_->setText(tr("unlock devices"));
                lockBtn_->setIcon(QIcon(":/res/images/ic_unlock_device.svg"));
            } else {
                lockBtn_->setText(tr("lock devices"));
                lockBtn_->setIcon(QIcon(":/res/images/ic_lock_device.svg"));
            }
        } else {
            lockBtn_->setIcon(QIcon(":/res/images/ic_lock_device_disable.svg"));
        }
    }
    if (resetBtn_) {
        resetBtn_->setEnabled(enabled);
        if(enabled) {
            resetBtn_->setIcon(QIcon(":/res/images/ic_restore_factory.svg"));
        } else {
            resetBtn_->setIcon(QIcon(":/res/images/ic_restore_factory_disable.svg"));
        }
    }
    if (upgradeBtn_) {
        upgradeBtn_->setEnabled(enabled);
        if(enabled) {
            upgradeBtn_->setIcon(QIcon(":/res/images/ic_update.svg"));
        } else {
            upgradeBtn_->setIcon(QIcon(":/res/images/ic_update_disable.svg"));
        }
    }
    if (screenCastBtn_) {
        screenCastBtn_->setEnabled(enabled);
        if(enabled) {
            screenCastBtn_->setIcon(QIcon(":/res/images/ic_screen_projection.svg"));
        } else {
            screenCastBtn_->setIcon(QIcon(":/res/images/ic_screen_projection_disable.svg"));
        }
    }
    if (screenMonitorBtn_) {
        screenMonitorBtn_->setEnabled(enabled);
        if(enabled) {
            screenMonitorBtn_->setIcon(QIcon(":/res/images/ic_screen_monitor.svg"));
        } else {
            screenMonitorBtn_->setIcon(QIcon(":/res/images/ic_screen_monitor_disable.svg"));
        }
    }
    update();
}

void CommonPluginUI::initActivateCover()
{
    activateCover_ = new QWidget(this);
    activateCover_->setAttribute(Qt::WA_AlwaysStackOnTop);
    activateCover_->setWindowFlag(Qt::FramelessWindowHint, true);
    activateCover_->setAttribute(Qt::WA_StyledBackground);
    activateCover_->setStyleSheet("background:rgba(0,0,0,0.82);");
    activateBtn_ = new QPushButton(tr("Activate and management"), this);
    activateBtn_->setObjectName("activateBtn");
    activateBtn_->setIconSize(QSize(34,34));
    activateBtn_->setIcon(QIcon(":/res/images/ic_activation.svg"));
    activateBtn_->setFixedSize(260, 56);
    activateBtn_->move(width() / 2 - 260 / 2, height() / 2 - 56 / 2);
    connect(activateBtn_, &QPushButton::clicked, this, &CommonPluginUI::activateRequest);
}

bool CommonPluginUI::deviceExecuting()
{
    for(CommonDeviceItem* device : classroom_->deviceItems()) {
        if(device->isExecuting()) {
            return true;
        }
    }
    return false;
}

QSet<QString> CommonPluginUI::allDevicesSNs() const
{
    QSet<QString> sns;
    auto devices = classroom_->deviceItems();
    for (auto device : devices) {
        sns.insert(device->sn());
    }
    return sns;
}

void CommonPluginUI::onBroadcastSignal()
{
    auto devices = classroom_->deviceItems();
    for(auto device : devices) {
        if(device && device->deviceConnected()) {
            device->addLostBroadcastCount();
        }
    }
}

void CommonPluginUI::onShareListChanged()
{
    for (auto deviceItem : classroom_->deviceItems()) {
        deviceItem->sendSharedFileList();
    }
}
