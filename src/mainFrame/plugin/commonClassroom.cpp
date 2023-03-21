#include "commonClassroom.h"

#include "devicePluginInterface.h"
#include "commonDeviceItem.h"
#include "logHelper.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRadioButton>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QTcpSocket>
#include <QDebug>

#include "graphicstextitem.h"
#include "graphicsscene.h"
#include "devicemanagement.pb.h"
#include "backgroundwidget.h"
#include "commondialog.h"

QMap<CommonDeviceItem*,QRectF> CommonClassroom::deviceMap_;

CommonClassroom::CommonClassroom(const DevPluginInterface *plugin, QWidget *parent)
    : QFrame(parent)
    , plugin_(plugin)
    , scene_(new GraphicsScene(this))
    , view_(new QGraphicsView)
    , resetDeviceCount_(0)
    , resetSuccCount_(0)
    , resetFailCount_(0)
{
    setStyleSheet("font-family: Microsoft YaHei;");
    view_->setDragMode(QGraphicsView::RubberBandDrag);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    backgroundWidget_ = new BackgroundWidget(this);
    connect(backgroundWidget_,&BackgroundWidget::addBtnClicked,
            this,&CommonClassroom::addBtnClicked);
    auto layout = new QVBoxLayout(this);
    selectAll_ = new QCheckBox(tr("Select All"),this);
    selectAll_->setObjectName("selectAll");
    setSelectAllChecked(false);
    connect(scene_,&GraphicsScene::boxChooseBegin,[this]{
        boxChoose_ = true;
    });
    connect(scene_,&GraphicsScene::boxChooseEnd,[this]{
        boxChoose_ = false;
        auto deviceList = scene_->selectedItems();
        for (auto item : deviceList) {
            if (auto device = dynamic_cast<CommonDeviceItem*>(item)) {
                device->setSelectedAndSignal(true);
            }
        }
    });
    connect(scene_,&GraphicsScene::selectionChanged,[this]{
        auto deviceList = deviceItems();
        if(!editLocked_) {
            if(deviceList.size() == sceneSelectedDeviceItems().size()) {
                selectAll_->setChecked(true);
                selectAll_->setText(tr("Deselect All"));
            } else {
                selectAll_->setChecked(false);
                selectAll_->setText(tr("Select All"));
            }
            for(auto device : deviceList) {
                if(device->isSelected()) {
                    device->setZValue(1);
                } else {
                    device->setZValue(0);
                }
            }
        }
    });
    connect(selectAll_,&QCheckBox::clicked,[this](bool checked){
        if(checked) {
            selectAll_->setText(tr("Deselect All"));
        } else {
            selectAll_->setText(tr("Select All"));
        }
        if(editLocked_) {
            auto items = connectedDeviceItems();
            for(auto device : items) {
                device->setSelectedAndSignal(checked);
            }
        } else {
            auto items = deviceItems();
            for(auto device : items) {
                device->setSelected(checked);
            }
        }
        view_->viewport()->update();
    });
    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(selectAll_);
    QSpacerItem* horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hLayout->addItem(horizontalSpacer);
    selectionNote_ = new QLabel(tr("Hold down the ctrl key to select multiple."),this);
    selectionNote_->setStyleSheet("color: red;");
    selectionNote_->hide();
    hLayout->addWidget(selectionNote_);
    positionLock_ = new QPushButton(tr("Automatic sorting"),this);
    positionLock_->setLayoutDirection(Qt::RightToLeft);
    positionLock_->setObjectName("positionLockBtn");
    positionLock_->setIconSize(QSize(36,18));
    positionLock_->setIcon(QIcon(":/res/images/ic_sort_on.svg"));
    positionLock_->setMinimumSize(100,24);
    hLayout->addWidget(positionLock_);
    auto margins = hLayout->contentsMargins();
    margins.setLeft(20);
    margins.setRight(10);
    margins.setBottom(0);
    margins.setTop(0);
    hLayout->setContentsMargins(margins);
    connect(positionLock_, &QPushButton::clicked, this, &CommonClassroom::onPositionLockClicked);
    connect(scene_,&QGraphicsScene::selectionChanged,[this]{
        emit selectionChanged();
    });
    view_->setScene(scene_);
    layout->addLayout(hLayout);
    layout->addWidget(view_);
    layout->setSpacing(0);
    checkDevicesNum();
}

void CommonClassroom::addDeviceItem(const QString& sn,
                                    const QString& id,
                                    double x,
                                    double y,
                                    uint32_t status)
{
    CommonDeviceItem *deviceItem = new CommonDeviceItem(this, plugin_);
    connect(deviceItem,&CommonDeviceItem::deleteDevice,this,&CommonClassroom::delDevice);
    connect(deviceItem,&CommonDeviceItem::deviceSelectionChanged,[this]{
        if(connectedDeviceItems().size() != selectedDeviceItems().size()) {
            setSelectAllChecked(false);
        } else {
            setSelectAllChecked(true);
        }
        emit selectionChanged();
    });
    connect(deviceItem,&CommonDeviceItem::devicePressed,[this]{
        auto items = scene_->selectedItems();
        if(items.empty()) {
            return;
        }
        qreal xmin = items.first()->pos().x();
        qreal xmax = items.first()->pos().x();
        qreal ymin = items.first()->pos().y();
        qreal ymax = items.first()->pos().y();

        for(auto item : items) {
            xmin = qMin(item->pos().x(),xmin);
            xmax = qMax(item->pos().x(),xmax);
            ymin = qMin(item->pos().y(),ymin);
            ymax = qMax(item->pos().y(),ymax);
        }
        qreal boxWidth = xmax - xmin + CommonDeviceItem::kDeviceItemWidth;
        qreal boxHeight = ymax - ymin + CommonDeviceItem::kDeviceItemHeight;

        deviceMap_.clear();

        for(auto item : items) {
            qreal deltaX1 = item->pos().x() - xmin;
            qreal deltaX2 = boxWidth - deltaX1;
            qreal deltaY1 = item->pos().y() - ymin;
            qreal deltaY2 = boxHeight - deltaY1;

            deviceMap_.insert((CommonDeviceItem*)item,QRectF(deltaX1,deltaY1,deltaX2,deltaY2));
        }
        qDebug() << "moveStart:" << deviceMap_;
    });
    connect(deviceItem, &CommonDeviceItem::cmdExecutionOver, [this](const QString& sn) {
        LOG(INFO) << "cmdExecutionOver, sn:" << sn.toStdString();
        if (resetDevices_.contains(sn) && resetDevices_[sn]->cmd == dm::CmdPackage_CMDTYPE_RESET) {
            QSet<QString> executeOverDevices;
            for (auto device : resetDevices_) {
                if (!device->isExecuting()) {
                    executeOverDevices.insert(device->sn());
                    if (device->status == DeviceStatusCode::kNormalState) {
                        ++resetSuccCount_;
                        LOG(INFO) << "after ++resetSuccCount_, resetSuccCount_=" << resetSuccCount_;
                    } else {
                        ++resetFailCount_;
                        LOG(INFO) << "after ++resetFailCount_, resetFailCount_=" << resetFailCount_;
                    }
                }
            }
            for (auto sn : executeOverDevices) {
                resetDevices_.remove(sn);
            }
            LOG(INFO) << "resetDeviceCount_=" << resetDeviceCount_ << " resetSuccCount_=" << resetSuccCount_ << " resetFailCount_=" << resetFailCount_;
            if ((resetSuccCount_ + resetFailCount_) == resetDeviceCount_) {
                CommonDialog* dialog = new CommonDialog(tr("System Prompt"),CommonDialog::OnlyOkButton,this);
                dialog->setDisplayText(tr("succ:%1,fail:%2").arg(resetSuccCount_).arg(resetFailCount_));
                dialog->setOk2BtnText(tr("I known"));
                dialog->show();
            }
        }
    });
    deviceItem->setSelected(true);
    deviceItem->setFlag(QGraphicsItem::ItemIsMovable, !editLocked_);
    deviceItem->setSnAndDeviceName(sn, id);
    deviceItem->setX(x);
    deviceItem->setY(y);
    deviceItem->status = status;
    scene_->addItem(deviceItem);
}

void CommonClassroom::load(const QJsonObject &classroomData)
{
    QJsonArray devicesArray = classroomData[ClassroomFileKey::Devices].toArray();
    for (auto jsonValue : devicesArray) {
        auto deviceJsonObj = jsonValue.toObject();
        addDeviceItem(deviceJsonObj[ClassroomFileKey::DeviceSN].toString(),
                      deviceJsonObj[ClassroomFileKey::DeviceID].toString(),
                      deviceJsonObj[ClassroomFileKey::DevicePosX].toDouble(),
                      deviceJsonObj[ClassroomFileKey::DevicePosY].toDouble(),
                      DeviceStatusCode::kNormalState/*deviceJsonObj[ClassroomFile::DeviceStatusKey].toInt()*/);
    }
    checkDevicesNum();
}

void CommonClassroom::save(QJsonObject &classroomData) const
{
    auto devices = deviceItems();
    QJsonArray devicesArray;
    for (auto &device : devices) {
        QJsonObject deviceJsonObj;
        deviceJsonObj[ClassroomFileKey::DeviceID] = device->deviceName();
        deviceJsonObj[ClassroomFileKey::DeviceSN] = device->sn();
        deviceJsonObj[ClassroomFileKey::DevicePosX] = device->x();
        deviceJsonObj[ClassroomFileKey::DevicePosY] = device->y();
        deviceJsonObj[ClassroomFileKey::DeviceStatus] = device->status;
        devicesArray.append(deviceJsonObj);
    }
    classroomData[ClassroomFileKey::Devices] = devicesArray;
}

void CommonClassroom::addDevices(const QList<QPair<QString, QString>> &sns2ids)
{
    auto exisitDevices = deviceItems();
    int size = exisitDevices.size();
    int i = size % 8;
    int j = size / 8;
    double stepX = 150;
    double stepY = 110;
    double itemBottom = 0;
    auto devices = deviceItems();
    for (auto device : devices) {
        if(device->boundingRect().bottom() > itemBottom) {
            itemBottom = device->boundingRect().bottom();
        }
    }
    for (auto sn2id : sns2ids) {
        if (exisitDevices.contains(sn2id.first)) {
            LOG(INFO) << "device with sn " << sn2id.first.toStdString() << " exisits";
            continue;
        }
        double x = i++ * stepX + 15;
        double y = j * stepY + 15;
        if(i % 8 == 0) {
            i = 0;
            j++;
        }
        addDeviceItem(sn2id.first, sn2id.second, x, y, 0);
    }
    checkDevicesNum();
}

void CommonClassroom::delDevice(const QString& sn)
{
    for (auto &item : scene_->items()) {
        qDebug() << item;
        auto device = dynamic_cast<CommonDeviceItem*>(item);
        if (device && device->sn() == sn) {
            scene_->removeItem(device);
            device->deleteLater();
            if(editLocked_) {
                sortDevices();
            }
            emit deviceDeleted();
            LOG(INFO) << sn.toStdString() << " deleted.";
            break;
        }
    }
    checkDevicesNum();
}

void CommonClassroom::checkDevicesNum()
{
    if (deviceItems().empty()) {
        backgroundWidget_->setEnabled(true);
        backgroundWidget_->show();
        backgroundWidget_->raise();
    } else {
        backgroundWidget_->setEnabled(false);
        backgroundWidget_->hide();
    }
}

QMap<QString, CommonDeviceItem *> CommonClassroom::deviceItems() const
{
    QMap<QString, CommonDeviceItem*> devices;
    if (scene_) {
        for (auto &item : scene_->items()) {
            if (auto device = dynamic_cast<CommonDeviceItem*>(item)) {
                devices.insert(device->sn(), device);
            }
        }
    }
    return devices;
}

QMap<QString, CommonDeviceItem *> CommonClassroom::selectedDeviceItems() const
{
    auto connectedDevices = connectedDeviceItems();
    QMap<QString, CommonDeviceItem*> selectedDevices;
    for (auto device : connectedDevices) {
        //规则：离线无法选中，需要复合判断才行
        if (device->readyAndSelected()) {
            selectedDevices.insert(device->sn(), device);
        }
    }
    return selectedDevices;
}

QMap<QString, CommonDeviceItem *> CommonClassroom::sceneSelectedDeviceItems() const
{
    QMap<QString, CommonDeviceItem*> devices;
    if (scene_) {
        for (auto item : scene_->items()) {
            auto device = dynamic_cast<CommonDeviceItem*>(item);
            if (device && device->isSelected()) {
                devices.insert(device->sn(), device);
            }
        }
    }
    return devices;
}

QMap<QString, CommonDeviceItem*> CommonClassroom::connectedDeviceItems() const
{
    QMap<QString, CommonDeviceItem*> connectedDevices;
    if (scene_) {
        for (auto &item : scene_->items()) {
            auto device = dynamic_cast<CommonDeviceItem*>(item);
            if (device && device->deviceReady()) {
                connectedDevices.insert(device->sn(), device);
            }
        }
    }
    return connectedDevices;
}

bool CommonClassroom::selectedDevicesAllLocked() const
{
    bool allLocked = true;
    auto items = selectedDeviceItems();
    if (items.empty()) {
        return false;
    }
    for (auto item : items) {
        if (!item->locked()) {
            allLocked = false;
            break;
        }
    }
    return allLocked;
}

void CommonClassroom::resizeEvent(QResizeEvent* /*e*/)
{
    backgroundWidget_->setFixedSize(view_->size());
    backgroundWidget_->move(view_->geometry().topLeft());

    bool windowMaximized = window()->isMaximized();
    auto rect = view_->geometry();
//    qDebug() << __FUNCSIG__ << view_->geometry() << view_->viewport()->geometry();
    auto screenSize = screen()->availableGeometry();
//    qreal scaleX = screenSize.width() / qreal(windowFixedWidth);
//    qreal scaleY = screenSize.height() / qreal(windowFixedHeight);
    qreal scaleX = screenSize.width() / qreal(screenSize.width() * 78 / 100);
    qreal scaleY = screenSize.height() / qreal(screenSize.height() * 71 / 100);
    //判断窗口是否已经调整大小完成或者回到固定大小
    if (windowMaximized) {//最大化，窗口只有两种状态，固定大小或最大化
        QMatrix matrix;
        matrix.scale(scaleX, scaleY);
        view_->setMatrix(matrix);
    } else {
        view_->resetMatrix();
        //由于view设置了缩放比例，导致scene的rect会超出viewport，需要预先调整调整
        scene_->setSceneRect(0,0,
                             qRound(rect.width()-(scaleX-1)*rect.x()),
                             qRound(rect.height()-(scaleY-1)*rect.y()));
//        qDebug() << __FUNCSIG__ << scene_->sceneRect();
    }
}

void CommonClassroom::resetDevices(bool accepted, int saveUserData)
{
    if (accepted) {
        resetDevices_ = selectedDeviceItems();
        resetDeviceCount_ = resetDevices_.size();
        resetSuccCount_ = 0;
        resetFailCount_ = 0;
        dm::CmdPackage cmdPkg;
        cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_RESET);
        cmdPkg.set_saveuserdata(saveUserData);
        dm::TransMsg transPkg;
        transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
        transPkg.set_packagecontent(cmdPkg.SerializeAsString());
        auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
        for (auto &selectedDevice : resetDevices_) {
            selectedDevice->executeCommand(dm::CmdPackage_CMDTYPE_RESET, data, 70*60);
        }
    }
}

void CommonClassroom::lockDevices(bool targetLockState)
{
    LOG(INFO) << "target lock state:" << targetLockState;
    dm::CmdPackage cmdPkg;
    cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_LOCK);
    cmdPkg.set_lock(targetLockState);
    dm::TransMsg transPkg;
    transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
    transPkg.set_packagecontent(cmdPkg.SerializeAsString());
    auto data = QByteArray::fromStdString(transPkg.SerializeAsString());
    for (auto &selectedDevice : selectedDeviceItems()) {
        if (selectedDevice->locked() != targetLockState) {
            LOG(INFO) << "device " << selectedDevice->sn().toStdString() << " lock state is " << selectedDevice->locked() << ", send cmd";
            selectedDevice->setLocked(targetLockState);
            selectedDevice->executeCommand(dm::CmdPackage_CMDTYPE_LOCK, data, 0);
        } else {
            LOG(INFO) << "device " << selectedDevice->sn().toStdString() << " lock state is " << selectedDevice->locked() << ", won't send cmd";
        }
    }
}


void CommonClassroom::shutdownDevices()
{
    dm::CmdPackage cmdPkg;
    cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_SHUTDOWN);
    dm::TransMsg transPkg;
    transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
    transPkg.set_packagecontent(cmdPkg.SerializeAsString());
    auto data = QByteArray::fromStdString(transPkg.SerializeAsString());

    for (auto &selectedDevice : selectedDeviceItems()) {
        selectedDevice->executeCommand(dm::CmdPackage_CMDTYPE_SHUTDOWN,data,2*60);
    }
}
void CommonClassroom::rebootDevices()
{
    dm::CmdPackage cmdPkg;
    cmdPkg.set_cmd(dm::CmdPackage_CMDTYPE_REBOOT);
    dm::TransMsg transPkg;
    transPkg.set_packagetype(dm::TransMsg_PACKAGETYPE_CMDPACKAGE);
    transPkg.set_packagecontent(cmdPkg.SerializeAsString());
    auto data = QByteArray::fromStdString(transPkg.SerializeAsString());

    for (auto &selectedDevice : selectedDeviceItems()) {
        selectedDevice->executeCommand(dm::CmdPackage_CMDTYPE_REBOOT,data,5*60);
    }
}

void CommonClassroom::setPositionLocked(bool locked)
{
    editLocked_ = locked;
    auto devices = deviceItems();
    if(editLocked_) {
        positionLock_->setIcon(QIcon(":/res/images/ic_sort_on.svg"));
        sortDevices();
        for(auto device : devices) {
            device->setFlag(QGraphicsItem::ItemIsMovable,false);
        }
        if(connectedDeviceItems().size() == selectedDeviceItems().size()) {
            setSelectAllChecked(true);
        } else {
            setSelectAllChecked(false);
        }
    } else {
        scene_->clearSelection();
        positionLock_->setIcon(QIcon(":/res/images/ic_sort_off.svg"));
        for(auto device : devices) {
            device->setFlag(QGraphicsItem::ItemIsMovable,true);
        }
        if(deviceItems().size() == sceneSelectedDeviceItems().size()) {
            setSelectAllChecked(true);
        } else {
            setSelectAllChecked(false);
        }
    }
}

void CommonClassroom::onPositionLockClicked()
{
    bool locked = !editLocked_;
    setPositionLocked(locked);
}

void CommonClassroom::setSelectAllChecked(bool checked)
{
    if(selectAll_) {
        selectAll_->setChecked(checked);
        if(checked) {
            selectAll_->setText(tr("Deselect All"));
        } else {
            selectAll_->setText(tr("Select All"));
        }
    }
}

void CommonClassroom::sortDevices()
{
    auto exisitDevices = deviceItems();
    if(exisitDevices.empty()) {
        return;
    }

    int i = 0;
    int j = 0;
    double stepX = 150;
    double stepY = 110;
    for(auto cit = exisitDevices.cbegin(); cit != exisitDevices.cend(); ++cit) {
        double x = i++ * stepX + 15;
        double y = j * stepY + 10;
        if(i % 8 == 0) {
            i = 0;
            j++;
        }
        auto deviceItem = cit.value();
        deviceItem->setX(x);
        deviceItem->setY(y);
    }
}
