#include "commonDeviceItem.h"

#include "fileTransferThread.h"
#include "devicemanagement.pb.h"
#include "devicePluginInterface.h"
#include "logHelper.h"
#include "md5.h"
#include "settings.h"
#include "config.h"
#include "fileDirHandler.h"
#include "devicedetail.h"
#include "graphicstextitem.h"
#include "commonClassroom.h"
#include "toastdialog.h"
#include "commondialog.h"
#include "warningwidget.h"
#include "transferconnectioncheckthread.h"

#include <QTcpSocket>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QRectF>
#include <QMenu>
#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QObject>
#include <QInputDialog>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QTimer>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

enum ErrorCode {
    kSuccess = 0,
    kNoSuchPlace,
    kNoEnoughSpace,
    kFileTransferAlreadyExists,
    kFileTransferInterrupt,
    kFileTransferSucc
};

CommonDeviceItem::CommonDeviceItem(CommonClassroom* classroom, const DevPluginInterface *plugin, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , plugin_(plugin)
    , classroom_(classroom)
    , charging_(false)
    , locked_(false)
    , executingMovie_(nullptr)
    , processCmdResult_(true)
    , fileTransferProgress_(0)
    , typeMismatch_(false)
{
    setAcceptHoverEvents(true);
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
    textItem_ = new GraphicsTextItem(this);
    textItem_->setPos(25,76);
    textItem_->setEnabled(false);  // 屏蔽直接编辑功能
    cmdExecutionTimeoutTimer_ = new QTimer(this);
    cmdExecutionTimeoutTimer_->setSingleShot(true);
    connect(cmdExecutionTimeoutTimer_,&QTimer::timeout,this,&CommonDeviceItem::onCmdExecutionTimeout);
    cmdResultIgnoreTimer_ = new QTimer(this);
    cmdResultIgnoreTimer_->setSingleShot(true);
    connect(cmdResultIgnoreTimer_, &QTimer::timeout, this, &CommonDeviceItem::onCmdResultIgnoreTimeout);

    auto itemImagesUrls = plugin_->deviceItemImagesUrls();
    auto dir = QDir(Settings::pluginsAbsPath());
    auto pluginsInfo = Config::pluginsInfo();
    if (dir.cd(plugin_->name() + "/" + pluginsInfo[plugin_->name()]) && dir.cd(Settings::VALUE_PLUGIN_RES_FOLDER_NAME) && itemImagesUrls.count() == DevPluginInterface::ImageType::CountNum) {
        enabledItemPixmap_ = QPixmap(dir.absoluteFilePath(itemImagesUrls[DevPluginInterface::ImageType::Normal])).scaled(QSize(60, 60), Qt::KeepAspectRatio);
        disabledItemPixmap_ = QPixmap(dir.absoluteFilePath(itemImagesUrls[DevPluginInterface::ImageType::Disable])).scaled(QSize(60, 60), Qt::KeepAspectRatio);
    } else {
        LOG(INFO) << "use default item images";
        enabledItemPixmap_ = QPixmap(":/res/images/ic_device_aibox.svg").scaled(QSize(60, 60), Qt::KeepAspectRatio);
        disabledItemPixmap_ = QPixmap(":/res/images/ic_device_aibox_disable.svg").scaled(QSize(60, 60), Qt::KeepAspectRatio);
    }
}

CommonDeviceItem::~CommonDeviceItem()
{
    if (cmdSocket) {
        cmdSocket->close();
        cmdSocket->deleteLater();
        cmdSocket = nullptr;
    }
    if (transferThread && transferThread->isRunning()) {
        transferThread->requestExit();
    }
}

void CommonDeviceItem::setCharging(bool charging)
{
    if (charging_ != charging) {
        charging_ = charging;
        emit chargingStateChanged(charging_);
    }
}

void CommonDeviceItem::setLocked(bool locked)
{
    if (locked_ != locked) {
        locked_ = locked;
        emit lockedStateChanged(locked_);
    }
}

void CommonDeviceItem::setVersion(const QString &version)
{
    if (version_ != version) {
        QString oldVersion = version_;
        version_ = version;
        emit versionChanged(oldVersion, version_);
    }
}

QString CommonDeviceItem::deviceName() const
{
    return name_;
}

void CommonDeviceItem::setTypeMismatch(bool typeMismatch)
{
    if (typeMismatch_ != typeMismatch) {
        typeMismatch_ = typeMismatch;
        if (typeMismatch) {
            setSnAndDeviceName(sn_, tr("unknown device type"));
        }
        else {
            setSnAndDeviceName(sn_, name_);
        }
    }
}

void CommonDeviceItem::setSnAndDeviceName(const QString &sn, const QString &devName)
{
    sn_ = sn;
    name_ = devName;
    textItem_->setDeviceName(name_.isEmpty() ? sn : name_, name_.isEmpty());
}

bool CommonDeviceItem::cmdSocketConnected() const
{
    return cmdSocket && cmdSocket->state() == QAbstractSocket::ConnectedState;
}

bool CommonDeviceItem::transferSocketConnected() const
{
    return transferSocket && transferSocket->state() == QAbstractSocket::ConnectedState;
}

int CommonDeviceItem::fileTransferProgress() const
{
    return fileTransferProgress_;
}

QRectF CommonDeviceItem::boundingRect() const
{
    qreal penWidth = 1;
    return QRectF(0 - penWidth / 2, 0 - penWidth / 2,
                kDeviceItemWidth + penWidth, kDeviceItemHeight + penWidth);
}

void CommonDeviceItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget)
{
    Q_UNUSED(item)
    Q_UNUSED(widget)

    painter->save();

    //draw background
    QColor bgColor(0x37, 0x3c, 0x49);
    if (!executing_ && (readyAndSelected_ || hover_ || isSelected())) {
        bgColor = QColor(0x44,0x4a,0x5a);
    }
    QPainterPath path;
    path.addRoundedRect(QRectF(0,0,kDeviceItemWidth,kDeviceItemHeight),2,2);
    painter->fillPath(path, bgColor);
    //draw pixmap
    if (typeMismatch_) {
        painter->drawPixmap(12, 10, 60, 60, QPixmap(":/res/images/ic_unknown_device.svg"));
        painter->drawPixmap(12, 81, 10, 10, QPixmap(":/res/images/ic_offline.svg"));
    }
    else if (deviceConnected()) {
        painter->drawPixmap(12,10,60,60, enabledItemPixmap_);
        painter->drawPixmap(80,37,22,10,QPixmap(getBatteryImagePath()));
        if (readyAndSelected_) {
            painter->drawPixmap(120,80,16,16,QPixmap(":/res/images/ic_checkbox_p.svg"));
        } else {
            painter->drawPixmap(120,80,16,16,QPixmap(":/res/images/ic_checkbox.svg"));
        }
        painter->drawPixmap(12,81,10,10,QPixmap(":/res/images/ic_online.svg"));
        painter->setPen(qRgba(255,255,255,204));
        QFont font("Microsoft YaHei");
        font.setPixelSize(10);
        painter->setFont(font);
        painter->drawText(QRectF(107, 35, 30, 14), QString("%1%").arg(power));
    }
    else {
        painter->drawPixmap(12,10,60,60,disabledItemPixmap_);
        painter->drawPixmap(12,81,10,10,QPixmap(":/res/images/ic_offline.svg"));
    }

    if (executing_ && executingMovie_ && executingMovie_->state() == QMovie::Running) {
        painter->drawPixmap(85,10,50,15, executingMovie_->currentPixmap());
    }

    if (locked_) {
        painter->drawPixmap(12,10,60,60,
                            QPixmap(":/res/images/ic_device_lock.svg"));
    }

    if (status != 0) {
        painter->drawPixmap(8,8,24,24,
                            QPixmap(":/res/images/ic_device_warning.svg"));
    }

    painter->restore();
}

void CommonDeviceItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    QAction *detailAction = menu.addAction(tr("Show Details"));
    connect(detailAction, &QAction::triggered, [this]() {
        showDetailDialog();
    });
    QAction *removeAction = menu.addAction(tr("Delete Device"));
    connect(removeAction, &QAction::triggered, [this]() {
        CommonDialog* dialog = new CommonDialog(tr("Delete Device"));
        dialog->setDisplayText(tr("Are you sure to delete this device?"));
        connect(dialog,&CommonDialog::sigAccepted,[this](bool accepted){
            if(accepted) {
                emit deleteDevice(sn_);
            }
        });
        dialog->show();
    });
//    menu.setMinimumSize(150,107);
//    menu.setMaximumSize(150,107);
    menu.setWindowFlags(menu.windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    menu.setStyleSheet("QMenu { border-radius: 4px; background-color: #505769; color:#FFFFFF; padding: 10px 10px;}"
                       "QMenu::item { font-size: 14px; padding: 4px 37px; margin: 10px 0px; border-radius: 4px;}"
                       "QMenu::item:selected { background-color: #00A5FF; }"
                       );
    menu.exec(event->screenPos());
}

void CommonDeviceItem::showDetailDialog()
{
    auto res = plugin_->obsoleteVersion("1.2.9.193");
    qDebug() << "res:" << res;
    bool cmd_connected = cmdSocket && (cmdSocket->state() == QAbstractSocket::ConnectedState);
    QString name = name_.isEmpty() ? sn_ : name_;
    DeviceDetail* detail = new DeviceDetail(plugin_, sn_, name, version_, power, getBatteryImagePath(), cmd_connected);
    connect(detail, &DeviceDetail::deviceNameEdited, [this](const QString& newName) {
        setSnAndDeviceName(sn_, newName);
    });
    detail->setStyleSheet("color:#FFFFFF");
    CommonDialog *dialog = new CommonDialog(tr("show detail"),CommonDialog::NoButton,classroom_);
    dialog->setMinimumSize(580,300);
    dialog->setMaximumSize(580,300);
    dialog->setDisplayWidget(detail);
    dialog->show();
//    detail->show();
}

void CommonDeviceItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (status != DeviceStatusCode::kNormalState) {
            QString statusStr = statusToString(status);
            warningwidget *warning = new warningwidget(statusStr);
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton);
            dialog->setMinimumSize(580,170);
            dialog->setDisplayWidget(warning);
            dialog->show();
        } else {
            showDetailDialog();
        }
    }
    isClickEvent_ = false;
    update();
}

void CommonDeviceItem::onCmdSocketReadyRead()
{
    if (cmdSocket && cmdSocket->state() == QAbstractSocket::ConnectedState) {
        auto data = cmdSocket->readAll();

        cmdSocket->peerAddress();
        cmdSocket->peerPort();

        parsePackage(cmdSocket->peerAddress() ,data);
    }
}

void CommonDeviceItem::onCmdSocketConnect()
{
    emit cmdConnectionStateChanged(true);
}

void CommonDeviceItem::onCmdSocketDisconnect()
{
    LOG(INFO) << "deivce " << sn_.toStdString() << " cmd socket disconnected";
    switch (cmd) {
    case dm::CmdPackage_CMDTYPE_REBOOT:
        break;
    case dm::CmdPackage_CMDTYPE_SHUTDOWN:
        setStatusAndSignal(0);
        break;
    default:
        break;
    }
//    setSelectedAndSignal(false);
    if (cmdSocket) {
        cmdSocket->close();
        cmdSocket->deleteLater();
        cmdSocket = nullptr;
    }
    ips.clear();
    update();

    emit cmdConnectionStateChanged(false);
}

void CommonDeviceItem::onTransferSocketConnect()
{
    LOG(INFO) << "CommonDeviceItem::onTransferSocketConnect sn:" << sn_.toStdString();
    emit transferConnectionStateChanged(true);
}

void CommonDeviceItem::onTransferSocketDisconnect()
{
    update();
    transferSocket = nullptr;
    LOG(INFO) << "CommonDeviceItem::onTransferSocketDisconnect sn:" << sn().toStdString();
    emit transferConnectionStateChanged(false);
}

void CommonDeviceItem::onTransferInterrupt()
{
    update();
    transferSocket = nullptr;
    LOG(INFO) << "CommonDeviceItem::onTransferInterrupt sn:" << sn_.toStdString();
    emit transferInterrupt();
}

void CommonDeviceItem::onTransferThreadFinished()
{
    LOG(INFO) << "CommonDeviceItem::onTransferThreadFinished sn:" << sn_.toStdString();
    update();
    transferThread = nullptr;
}

void CommonDeviceItem::onTransferProgress(qint64 val)
{
    if ((val < 0) || (val > 100)) {
        LOG(ERROR) << "invalid transfer progress:" << val;
        val = 0;
    }
    fileTransferProgress_ = val;
    emit fileTransferProgressed(val);
}

void CommonDeviceItem::sendFileInfo(const QString &fileName, bool isSharedFile)
{
    if (cmdSocket && cmdSocket->state() == QAbstractSocket::ConnectedState) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            qint64 size = file.size();
            QString md5;
            if (!isSharedFile) {
                UpgradePackageInfo upgradePackageInfo;
                plugin_->readUpgradePackageInfo(upgradePackageInfo);
                md5 = upgradePackageInfo.md5;
            }
            if (md5.isEmpty()) {
                md5 = MD5::fileMd5(fileName);
            }
            LOG(INFO) << "fileName:" << fileName.toStdString() << " size:" << size << " md5:" << md5.toStdString();
            dm::FileInfoPackage fileInfoPackage;
            QFileInfo fileInof(fileName);
            fileInfoPackage.set_filename(fileInof.fileName().toStdString());
            fileInfoPackage.set_md5(QString(md5).toStdString());
            fileInfoPackage.set_size(size);
            fileInfoPackage.set_other("0");
            fileInfoPackage.set_issharedfile(isSharedFile);
            dm::TransMsg transPkg;
            transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_FILEINFOPACKAGE);
            transPkg.set_packagecontent(fileInfoPackage.SerializeAsString());
            auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
            cmdSocket->write(data);
            cmdSocket->flush();
            file.close();
        }
    }
}


void CommonDeviceItem::transferfilePrivate()
{
    if (transferThread && transferThread->isRunning()) {
        LOG(INFO) << "transfer thread is running...";
        return;
    }
    LOG(INFO) << "start transfer thread";
    transferThread = new FileTransferThread(transferSocket, transferFileName_, transferFilePos_);
    transferSocket->moveToThread(transferThread);
    connect(transferThread, &QThread::finished, this, &CommonDeviceItem::onTransferThreadFinished);
    if (!transferFileIsSharedFile_) { // transfer upgrade image
        connect(transferThread, &FileTransferThread::transferProgress, this, &CommonDeviceItem::onTransferProgress);
        connect(transferThread, &FileTransferThread::transferConnected, this, &CommonDeviceItem::onTransferSocketConnect);
        connect(transferThread, &FileTransferThread::transferDisconnected, this, &CommonDeviceItem::onTransferSocketDisconnect);
        connect(transferThread, &FileTransferThread::transferInterrupt, this, &CommonDeviceItem::onTransferInterrupt);
    }
    fileTransferProgress_ = 0;
    transferThread->start();
    emit transferStart();
}

void CommonDeviceItem::transferFile(bool isSharedFile, const QString &fileName, qint64 pos)
{
    LOG(INFO) << "CommonDeviceItem::transferFile enter";
    transferFileIsSharedFile_ = isSharedFile;
    transferFileName_ = fileName;
    transferFilePos_ = pos;
    if (transferSocket && transferSocket->state() == QAbstractSocket::ConnectedState) {
        transferfilePrivate();
    } else {
        LOG(INFO) << "transferSocket is null or not connected";
        if(connectionCheckThread_ != nullptr) {
            connectionCheckThread_->deleteLater();
        }
        connectionCheckThread_ = new TransferConnectionCheckThread(this);
        connect(connectionCheckThread_,&TransferConnectionCheckThread::transferConnected,
                this,&CommonDeviceItem::transferfilePrivate);
        connectionCheckThread_->start();
    }
}

void CommonDeviceItem::stopFileTransfer()
{
    LOG(INFO) << "CommonDeviceItem::stopFileTransfe sn:" << sn_.toStdString();
    if (transferThread) {
        transferThread->requestExit();
    }
}

void CommonDeviceItem::sendSharedFileList()
{
    if (cmdSocket && cmdSocket->state() == QAbstractSocket::ConnectedState) {
        auto sharedFolderPath = Settings::sharedFolderAbsPath();
        dm::SharedFileListPackage sharedFileListPackage;
        QDir sharedFolder(sharedFolderPath);
        if (!sharedFolder.exists()) {
            sharedFolder.mkpath(".");
        } else {
            QFileInfoList fileInfoList = sharedFolder.entryInfoList(QDir::Files | QDir::NoSymLinks);
            for (auto &fileInfo : fileInfoList) {
                auto sharedFile = sharedFileListPackage.add_sharedfile();
                sharedFile->set_filename(fileInfo.fileName().toStdString());
                sharedFile->set_sharetimestamp(fileInfo.birthTime().toSecsSinceEpoch());
                sharedFile->set_size(fileInfo.size());
            }
        }
        dm::TransMsg transPkg;
        transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_SHAREDFILELISTPACKAGE);
        transPkg.set_packagecontent(sharedFileListPackage.SerializeAsString());
        auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
        cmdSocket->write(data);
        cmdSocket->flush();
    }
}

void CommonDeviceItem::startUpgradeProgcess(const UpgradePackageInfo &upgradePackageInfo, int saveUserData)
{
    if (cmdSocket && cmdSocket->state() == QAbstractSocket::ConnectedState) {
        setDeviceExecuting(true);
        if(cmdResultIgnoreTimer_->isActive()) {
            cmdResultIgnoreTimer_->stop();
        }
        cmdResultIgnoreTimer_->start(plugin_->cmdExcuteEstimateTime() * 1000);
        processCmdResult_ = false;
        if (plugin_->obsoleteVersion(version_)) {
            cmd = dm::CmdPackage_CMDTYPE_UPGRADE;
            status = DeviceStatusCode::kNormalState;
            dm::CmdPackage cmdPkg;
            cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_UPGRADE);
            cmdPkg.set_version(upgradePackageInfo.version.toStdString());
            cmdPkg.set_imgname(upgradePackageInfo.upgradeFileName.toStdString());
            cmdPkg.set_saveuserdata(saveUserData);
            dm::TransMsg transPkg;
            transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
            transPkg.set_packagecontent(cmdPkg.SerializeAsString());
            auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
            cmdSocket->write(data);
            cmdSocket->flush();
        }
        else {
            cmd = dm::CmdPackageV2_CMDTYPE_UPGRADE;
            status = DeviceStatusCode::kNormalState;
            dm::CmdPackageV2 cmdPkgV2;
            cmdPkgV2.set_cmd(dm::CmdPackageV2_CMDTYPE_UPGRADE);
            QJsonObject upgradeCmdParasObj;
            upgradeCmdParasObj["upgradeMode"] = saveUserData;
            upgradeCmdParasObj["fileName"] = upgradePackageInfo.upgradeFileName;
            upgradeCmdParasObj["fileSize"] = upgradePackageInfo.upgradeFileSize;
            upgradeCmdParasObj["fileMd5"] = upgradePackageInfo.md5;
            upgradeCmdParasObj["version"] = upgradePackageInfo.version;
            upgradeCmdParasObj["config"] = upgradePackageInfo.config;
            QJsonDocument upgradeCmdParasDoc;
            upgradeCmdParasDoc.setObject(upgradeCmdParasObj);
            cmdPkgV2.set_parameters(upgradeCmdParasDoc.toJson().toStdString());
            dm::TransMsg transPkg;
            transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE_V2);
            transPkg.set_packagecontent(cmdPkgV2.SerializeAsString());
            auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
            cmdSocket->write(data);
            cmdSocket->flush();
        }
    }
}

void CommonDeviceItem::onStartUpgradeProgcessRequest(const UpgradePackageInfo &upgradePackageInfo, int saveUserData)
{
    LOG(INFO) << "CommonDeviceItem::onStartUpgradeProgcessRequest sn:" << sn_.toStdString();
    startUpgradeProgcess(upgradePackageInfo, saveUserData);
}

void CommonDeviceItem::parsePackage(QHostAddress deviceAddress, const QByteArray &data)
{
    LOG(INFO) << "CommonDeviceItem::parsePackage sn:" << sn_.toStdString();
    dm::TransMsg transMsg;
    transMsg.ParseFromString(data.toStdString());
    switch(transMsg.packagetype()) {
    case dm::TransMsg_PACKAGETYPE_CMDPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_CMDPACKAGE";
    }
        break;
    case dm::TransMsg_PACKAGETYPE_BROADCASTPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_BROADCASTPACKAGE";
    }
        break;
    case dm::TransMsg_PACKAGETYPE_CONNECTPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_CONNECTPACKAGE";
    }
        break;
    case dm::TransMsg_PACKAGETYPE_RESPONSEPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_RESPONSEPACKAGE";
        dm::ResponsePackage pkg;
        pkg.ParseFromString(transMsg.packagecontent());
        LOG(INFO) << "RESPONSEPACKAGE:" << pkg.code() << ",msg:" << pkg.message();
        if(executing_ && pkg.code() == ExceptionCode::DeviceBusy) {
            if(cmd == dm::CmdPackage_CMDTYPE_RESET) {
                setStatusAndSignal(DeviceStatusCode::kResetFail); //TODO: 需要配合设备端一起改
            } else if(cmd == dm::CmdPackage_CMDTYPE_SHUTDOWN) {
                setStatusAndSignal(DeviceStatusCode::kShutDownFailed);
            } else if(cmd == dm::CmdPackage_CMDTYPE_REBOOT) {
                setStatusAndSignal(DeviceStatusCode::kRebootFailed);
            }
        }
    }
        break;
    case dm::TransMsg_PACKAGETYPE_STATEPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_STATEPACKAGE";
    }
        break;
    case dm::TransMsg_PACKAGETYPE_PERCENTPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_PERCENTPACKAGE";
    }
        break;
    case dm::TransMsg_PACKAGETYPE_FILEREQUESTPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_FILEREQUESTPACKAGE";
        dm::FileRequestPackage fileRequestPackage;
        fileRequestPackage.ParseFromString(transMsg.packagecontent());
        auto fileName = fileRequestPackage.filename();
        bool isSharedFile = fileRequestPackage.issharedfile();
        QString filePath;
        if (isSharedFile) {
            filePath = Settings::sharedFolderAbsPath();
        } else {
            filePath = Settings::deviceUpgradeFolderAbsPath(plugin_->name());
        }
        filePath = FileDirHandler::absolutePath(QString::fromStdString(fileName), filePath);
        LOG(INFO) << "fileName:" << fileName << " abs:" << filePath.toStdString();
        sendFileInfo(filePath, isSharedFile);
    }
        break;
    case dm::TransMsg_PACKAGETYPE_RECVREADYPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_RECVREADYPACKAGE";
        dm::RecvReadyPackage recvReadyPackage;
        recvReadyPackage.ParseFromString(transMsg.packagecontent());
        auto fileName = recvReadyPackage.filename();
        auto offset = recvReadyPackage.offset();
        bool isSharedFile = recvReadyPackage.issharedfile();
        QString filePath;
        if (isSharedFile) {
            filePath = Settings::sharedFolderAbsPath();
        } else {
            filePath = Settings::deviceUpgradeFolderAbsPath(plugin_->name());
        }
        filePath = FileDirHandler::absolutePath(QString::fromStdString(fileName), filePath);
        LOG(INFO) << "fileName:" << fileName << " abs:" << filePath.toStdString() << " offset:" << offset;
        transferFile(isSharedFile, filePath, offset);
    }
        break;
    case dm::TransMsg_PACKAGETYPE_REQUESTSHAREDFILELISTPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_REQUESTSHAREDFILELISTPACKAGE";
        sendSharedFileList();
    }
        break;
    case dm::TransMsg_PACKAGETYPE_UPGRADESTAGEPACKAGE: {
        LOG(INFO) << "TransMsg_PACKAGETYPE_UPGRADESTAGEPACKAGE";
        dm::UpgradeStagePackage stagePackage;
        stagePackage.ParseFromString(transMsg.packagecontent());
        int deviceStageCode = stagePackage.stage();
        emit aboutToUpgrading(deviceStageCode);
    }
        break;
    default: {
        LOG(INFO) << "unknown msg";
    }
        break;
    }
}

QString CommonDeviceItem::statusToString(int32_t statusCode)
{
    switch (statusCode) {
    case DeviceStatusCode::kAlreadyConnected:
//        return tr("already connected to %1").arg(connectedServerIp);
        return tr("network error, please check and try again");
    case DeviceStatusCode::kShutDownTimeout:
        return tr("shutdown timeout");
    case DeviceStatusCode::kShutDownFailed:
        return tr("shutdown failed");
    case DeviceStatusCode::kRebootTimeout:
        return tr("reboot timeout");
    case DeviceStatusCode::kRebootFailed:
        return tr("reboot failed");
    case DeviceStatusCode::kResetTimeout:
        return tr("reset timeout");
    case DeviceStatusCode::kResetNoCharging:
        return tr("reset no charging");
    case DeviceStatusCode::kResetFail:
        return tr("reset unknown fail");
    case DeviceStatusCode::kUpgradeTimeout:
        return tr("upgrade timeout");
    case DeviceStatusCode::kImgTransferFailed:
        return tr("upgrade transfer timeout");
    case DeviceStatusCode::kUpgradeFail:
        return tr("upgrade unknown fail");
    case DeviceStatusCode::kUpgradeNoEnoughSpace:
        return tr("upgrade no enough space");
    case DeviceStatusCode::kDeviceLocked:
        return tr("device locked");
    case DeviceStatusCode::kExecutingCmd:
        return tr("executing other cmd");
    case DeviceStatusCode::kDevBusy:
        return tr("device busy");
    case DeviceStatusCode::kDevObsoleteVersion:
        return tr("device version obsolete");
    default:
        break;
    }
    return tr("unknown error");
}

void CommonDeviceItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{

//    selected_ = !selected_;
//    selectedBeforeClick_= selected_;
//    selected_ = true;
//    isClickEvent_ = true;
    if(event->buttons().testFlag(Qt::LeftButton)) {
        pressPos_ = event->pos();
        isClickEvent_ = true;
        if (deviceConnected()) { //已连接设备非排版编辑模式才修改选中状态
            QTimer::singleShot(300,this,[this]{ //double-click需要500ms判断，选取一个大于500ms的时间
                if (isClickEvent_) {
                    isClickEvent_ = false;
                    setSelectedAndSignal(!readyAndSelected_);
                    update();
                }
            });
        }
    }
    QGraphicsObject::mousePressEvent(event);
    if(event->buttons().testFlag(Qt::LeftButton)) {
        emit devicePressed();
//        event->setModifiers(event->modifiers() | Qt::ControlModifier);
    }
}

void CommonDeviceItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
//    event->setModifiers(event->modifiers() | Qt::ControlModifier);
    QGraphicsObject::mouseReleaseEvent(event);
//    if(isClickEvent_) {
//        isClickEvent_ = false;
//        selected_ = !selectedBeforeClick_;
//        setSelected(selected_);
//        emit deviceSelectionStateChange();
//    }
//    update();
}

void CommonDeviceItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto pos = event->pos();
    //有时候点击会细微移动，导致错误的弹出toast，需要过滤
    if((qAbs(pos.x() - pressPos_.x()) > 5 || qAbs(pos.y() - pressPos_.y()) > 5)
            && isClickEvent_){
        isClickEvent_ = false;
        if(!flags().testFlag(ItemIsMovable)) {
            QWidget* wnd = classroom_->window();
            ToastDialog* toast = new ToastDialog(wnd);
            toast->setObjectName("lockToast");
            toast->move((wnd->width()-toast->width())/2,(wnd->height()-toast->height())/2);
            toast->show();
        }
    }
    QGraphicsObject::mouseMoveEvent(event);
}

QVariant CommonDeviceItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        // value is the new position.
        QPointF newPos = value.toPointF();
        QRectF thisMap = CommonClassroom::deviceMap_[this];
        QRectF rect = scene()->sceneRect();
        rect.setX(thisMap.x());
        rect.setY(thisMap.y());
        rect.setWidth(rect.width() - thisMap.width());
        rect.setHeight(rect.height() - thisMap.height());
        if (!rect.contains(newPos)) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            return newPos;
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

void CommonDeviceItem::startCmdTimer(int sec)
{
    if (sec <= 0) {
        return;
    }
    if(cmdExecutionTimeoutTimer_->isActive()) {
        cmdExecutionTimeoutTimer_->stop();
    }
    cmdExecutionTimeoutTimer_->start(sec*1000);
}

bool CommonDeviceItem::deviceConnected()
{
    bool connected = (cmdSocket && (cmdSocket->state() == QAbstractSocket::ConnectedState));
    return connected;
}

bool CommonDeviceItem::deviceReady()
{
    if (!executing_ && deviceConnected()) {
        return true;
    }

    return false;
}

void CommonDeviceItem::executeCommand(int cmdType, const QByteArray& cmdData, int timeoutSeconds)
{
    LOG(INFO) << "executeCommand cmdType:" << cmdType << " executing_:" << executing_ << " cmd:" << cmd;
    if ((cmdType != dm::CmdPackage_CMDTYPE_LOCK) && plugin_->exclusiveLock() && locked_) {
        LOG(INFO) << "device locked, and cmd is not lock/unlock";
        status = DeviceStatusCode::kDeviceLocked;
        return;
    }
    switch (cmdType) {
    case dm::CmdPackage_CMDTYPE_RESET:
    case dm::CmdPackage_CMDTYPE_UPGRADE:
        if(cmdResultIgnoreTimer_->isActive()) {
            cmdResultIgnoreTimer_->stop();
        }
        cmdResultIgnoreTimer_->start(plugin_->cmdExcuteEstimateTime() * 1000);
        processCmdResult_ = false;
//        if (!charging_) {
//            setStatusAndSignal(DeviceStatusCode::kResetNoCharging);
//            return;
//        }
        break;
    default:
        break;
    }
    cmd = cmdType;
    status = DeviceStatusCode::kNormalState;
//    setSelectedAndSignal(false);
    startCmdTimer(timeoutSeconds);
    auto ret = cmdSocket->write(cmdData);
    LOG(INFO) << "executeCommand ret:" << ret;
    cmdSocket->flush();
    setDeviceExecuting(true);
    update();
}

void CommonDeviceItem::setStatusAndSignal(int statusCode)
{
    LOG(INFO) << sn_.toStdString() << " setStatusAndSignal " << statusCode;
    if(cmdExecutionTimeoutTimer_->isActive()) {
        cmdExecutionTimeoutTimer_->stop();
    }
    status = statusCode;
    setDeviceExecuting(false);
    setSelectedAndSignal(true);
}

void CommonDeviceItem::onDeviceStatusChange(int deviceState)
{
    LOG(INFO) <<  sn_.toStdString() <<  "onDeviceStatusChange " << deviceState;
    if (!processCmdResult_) {
        LOG(INFO) << "processCmdResult_" << processCmdResult_;
        return;
    }
    switch (cmd) {
    case dm::CmdPackage_CMDTYPE_RESET: {
        switch (deviceState) {
        case DeviceRestoreSuccess: {
            setStatusAndSignal(kNormalState);
        }
            break;
        case DeviceRestoreFail: {
            setStatusAndSignal(kResetFail);
        }
            break;
        case DeviceRestoreFail_AdapterUnplug: {
            setStatusAndSignal(kResetNoCharging);
        }
            break;
        default: {
            LOG(ERROR) << "unknown deviceState:" << deviceState;
        }
            break;
        }
    }
        break;
    case dm::CmdPackage_CMDTYPE_UPGRADE: {
        if (deviceState == DeviceUpdateSuccess) {
            setDeviceExecuting(false);
            emit upgradeResultBroadcast(true);
        } else if (deviceState == DeviceUpdateFail) {
            setDeviceExecuting(false);
            emit upgradeResultBroadcast(false);
        }
    }
        break;
    }
}

void CommonDeviceItem::onLockStatusResponse(bool deviceLockStatus)
{
    LOG(INFO) << "deviceLockStatus:" << deviceLockStatus << " locked_:" << locked_;
    if (locked_ == deviceLockStatus) {
        if (cmd == dm::CmdPackage_CMDTYPE_LOCK) {
            setDeviceExecuting(false);
            setSelectedAndSignal(readyAndSelected_);
        } else {
            LOG(INFO) << "cmd " << cmd;
        }
    } else {
        if (cmdSocket && cmdSocket->state() == QAbstractSocket::ConnectedState) {
            dm::CmdPackage cmdPkg;
            cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_LOCK);
            cmdPkg.set_lock(locked_);
            dm::TransMsg transPkg;
            transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
            transPkg.set_packagecontent(cmdPkg.SerializeAsString());
            auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
            executeCommand(dm::CmdPackage_CMDTYPE_LOCK, data, 0);
        } else {
            LOG(INFO) << "cmdSocket is null or cmdSocket state is not ConnectedState";
        }
    }
}

void CommonDeviceItem::setSelectedAndSignal(bool selected)
{
    if (selected) {
        if (deviceReady()) {
            readyAndSelected_ = selected;
            emit deviceSelectionChanged();
        }
    } else {
        readyAndSelected_ = selected;
        emit deviceSelectionChanged();
    }
}

void CommonDeviceItem::setDeviceExecuting(bool executing)
{
    LOG(INFO) << "setDeviceExecuting " << sn_.toStdString() << " " << executing << " before: " << executing_;
    executing_ = executing;
    update();
    if (!executing_) {
        emit cmdExecutionOver(sn_);
        if (executingMovie_) {
            executingMovie_->deleteLater();
            executingMovie_ = nullptr;
        }
    } else {
        executingMovie_ = new QMovie(":/res/images/executing.gif");
        connect(executingMovie_,&QMovie::frameChanged,
                this,&CommonDeviceItem::onMovieFrameChanged);
        executingMovie_->start();
    }
}

QString CommonDeviceItem::getBatteryImagePath()
{
    QString url = ":/res/images/ic_power_80%.svg";
    if(power <= 20) {
        url = ":/res/images/ic_power_20%.svg";
    } else if(20 < power && power <= 40) {
        url = ":/res/images/ic_power_40%.svg";
    } else if(40 < power && power <= 60) {
        url = ":/res/images/ic_power_60%.svg";
    } else if(60 < power && power <= 80) {
        url = ":/res/images/ic_power_80%.svg";
    } else if(80 < power && power < 100) {
        url = ":/res/images/ic_power_80%.svg";
    } else if(power == 100) {
        url = ":/res/images/ic_power_100%.svg";
    }
    return url;
}

void CommonDeviceItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    hover_ = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
}
void CommonDeviceItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    hover_ = false;
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}

void CommonDeviceItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if(deviceConnected()) {
        QRectF rect{80,37,22,10};
        if(rect.contains(event->pos()) && power < 20) {
           setToolTip(tr("Device is low, please charge!"));
        } else {
            setToolTip("");
        }
    } else {
        setToolTip("");
    }
    QGraphicsObject::hoverMoveEvent(event);
}

void CommonDeviceItem::onMovieFrameChanged(int frame)
{
    Q_UNUSED(frame)
    update(85,10,50,15);
}

void CommonDeviceItem::onCmdExecutionTimeout()
{
    LOG(INFO) << sn_.toStdString() << "onCmdExecutionTimeout " << executing_ << " " << cmd;
    if (executing_) {
        switch (cmd) {
        case dm::CmdPackage_CMDTYPE_SHUTDOWN:
            status = DeviceStatusCode::kShutDownTimeout;
            break;
        case dm::CmdPackage_CMDTYPE_REBOOT:
            status = DeviceStatusCode::kRebootTimeout;
            break;
        case dm::CmdPackage_CMDTYPE_RESET:
            status = DeviceStatusCode::kResetTimeout;
            break;
        default:
            break;
        }
        setDeviceExecuting(false);
        update();
    }
}

void CommonDeviceItem::onCmdResultIgnoreTimeout()
{
    processCmdResult_ = true;
}

void CommonDeviceItem::addLostBroadcastCount()
{
    ++lostBroadcastCount_;
    LOG(INFO) << "CommonDeviceItem::addLostBroadcastCount sn:" << sn_.toStdString() << " lostBroadcastCount_:" << lostBroadcastCount_;
    if (lostBroadcastCount_ == 4) { //前3次未收到，断开连接
        if (cmdSocket) {
            cmdSocket->close();
            cmdSocket->deleteLater();
            cmdSocket = nullptr;
        }
        if (transferThread && transferThread->isRunning()) {
            LOG(INFO) << "CommonDeviceItem::addLostBroadcastCount transferThread->requestExit, sn:" << sn_.toStdString();
            transferThread->requestExit();
        }
    }
}

void CommonDeviceItem::resetLostBroadcastCount()
{
    lostBroadcastCount_ = 0;
}

