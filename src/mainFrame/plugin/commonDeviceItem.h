#ifndef COMMONDEVICEITEM_H
#define COMMONDEVICEITEM_H

#include "pluginGlobal.h"
#include "toastdialog.h"
#include "devicePluginInterface.h"

#include <QGraphicsObject>
#include <QHostAddress>
#include <QSet>
#include <QMovie>
#include <QPixmap>

enum ExceptionCode {
    NoException = 0,
    NoSuchPlace,
    NoEnoughSpace,
    FileTransferAlreadyExists,
    FileTransferInterrupt,
    FileTransferSucc,
    FileTransferFail,
    DeviceBusy,          // 设备忙
    ObsoleteVersion,     // 过时版本
    UpgradeFail
};

enum DeviceState
{
    DeviceIdle = 0,             // 空闲状态

    DeviceToUpdate = 1000,      // 升级中
    DeviceUpdateSuccess,        // 升级成功
    DeviceUpdateFail,           // 升级失败
    DeviceUpdateFail_Busy,      // 设备忙导致无法升级

    DeviceToRestore = 2000,          // 恢复系统中
    DeviceRestoreSuccess,            // 恢复系统成功
    DeviceRestoreFail,               // 恢复系统失败
    DeviceRestoreFail_AdapterUnplug, // 未插电导致恢复失败

    DeviceFileSendingPause = 3000,   // 文件发送暂停
    DeviceFileSendingResume,         // 文件发送恢复
    DeviceFileSendingSuccess,        // 文件发送成功
    DeviceFileSendingFail,           // 文件发送失败
};

enum DeviceStatusCode {
    kNormalState = 0,
    kAlreadyConnected = 1,

    // shutdown
    kShutDownTimeout = 10,
    kShutDownFailed = 11,

    // reboot
    kRebootTimeout = 20,
    kRebootFailed = 21,

    // reset
    kResetTimeout = 30,
    kResetNoCharging = 31,
    kResetFail = 32,

    // upgrade
    kUpgradeTimeout = 40,
    kImgTransferFailed = 41,
    kUpgradeFail = 42,
    kUpgradeNoEnoughSpace = 43,
    kDeviceLocked = 44,
    kExecutingCmd = 45,
    kDevBusy = 99,
    kDevObsoleteVersion = 100,

};

class QTcpSocket;
class FileTransferThread;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class GraphicsTextItem;
class CommonClassroom;
class DevPluginInterface;
class TransferConnectionCheckThread;

class PLUGIN_EXPORT CommonDeviceItem : public QGraphicsObject
{
    Q_OBJECT

public:
    int32_t               status = DeviceStatusCode::kNormalState;
    int32_t               cmd = 0;
    QSet<QHostAddress>    ips;
    int                   power = 0;
    QTcpSocket            *cmdSocket = nullptr;
    QTcpSocket            *transferSocket = nullptr;
    FileTransferThread    *transferThread = nullptr;
    GraphicsTextItem      *textItem_;
    QString               connectedServerIp;

    static const int kDeviceItemWidth = 140;
    static const int kDeviceItemHeight = 100;

public:
    explicit CommonDeviceItem(CommonClassroom* classroom, const DevPluginInterface *plugin, QGraphicsItem *parent = nullptr);
    ~CommonDeviceItem();

    bool charging() const { return charging_; }
    void setCharging(bool charging);
    bool locked() const { return locked_; }
    void setLocked(bool locked);
    QString version() const { return version_; }
    void setVersion(const QString &version);
    QString deviceName() const;
    QString sn() const { return sn_; }
    void setTypeMismatch(bool typeMismatch);
    bool typeMismatch() const { return typeMismatch_; }
    void setSnAndDeviceName(const QString &sn, const QString &devName);
    bool cmdSocketConnected() const;
    bool transferSocketConnected() const;
    int fileTransferProgress() const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) override;

    void onCmdSocketReadyRead();
    void onCmdSocketConnect();
    void onCmdSocketDisconnect();

    void onTransferSocketConnect();
    void onTransferSocketDisconnect();
    void onTransferInterrupt();
    void onTransferThreadFinished();
    void onTransferProgress(qint64 val);

    void sendFileInfo(const QString &fileName, bool isSharedFile);
    void transferFile(bool isSharedFile, const QString &fileName, qint64 pos = 0);
    void stopFileTransfer();
    void sendSharedFileList();
    void startUpgradeProgcess(const UpgradePackageInfo &upgradePackageInfo, int saveUserData);
    void onStartUpgradeProgcessRequest(const UpgradePackageInfo &upgradePackageInfo, int saveUserData);

    void parsePackage(QHostAddress deviceAddress, const QByteArray &data);
    QString statusToString(int32_t statusCode);

    void showDetailDialog();
    void startCmdTimer(int sec);
    bool deviceConnected();
    bool deviceReady();

    void executeCommand(int cmdType, const QByteArray& cmdData, int timeoutSeconds);
    void setStatusAndSignal(int statusCode);
    void onDeviceStatusChange(int deviceState);
    void onLockStatusResponse(bool deviceLockStatus);
    void setSelectedAndSignal(bool selected);
    bool readyAndSelected() { return readyAndSelected_; }
    bool isExecuting() { return executing_; }
    void setDeviceExecuting(bool executing);

    QString getBatteryImagePath();
    void onMovieFrameChanged(int frame);
    void onCmdExecutionTimeout();
    void onCmdResultIgnoreTimeout();

    void addLostBroadcastCount();
    void resetLostBroadcastCount();

signals:
    void deleteDevice(const QString& sn);
    void deviceSelectionChanged();
    void devicePressed();
    void cmdExecutionOver(const QString& sn);

    void cmdConnectionStateChanged(bool newState);
    void transferConnectionStateChanged(bool newState);
    void transferStart();
    void transferInterrupt();
    void versionChanged(QString oldVersion, QString newVersion);
    void chargingStateChanged(bool newStatus);
    void lockedStateChanged(bool newLockedState);

    void fileTransferProgressed(qint64 val);
    void upgradeResultBroadcast(bool success);

    void aboutToUpgrading(int);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void transferfilePrivate();

private:
    const DevPluginInterface *plugin_;
    QPixmap enabledItemPixmap_;
    QPixmap disabledItemPixmap_;

    QPointF pressPos_;
    bool isClickEvent_ = true; //默认认为是单击事件，检测到双击事件就清掉标记
    bool selectedBeforeClick_ = true;
    CommonClassroom* classroom_;

    QString sn_;
    QString name_;
    QString version_;
    bool    charging_;
    bool    locked_;
    qint64  fileTransferProgress_;
    bool    typeMismatch_;

    bool            readyAndSelected_ = false; //ready的含义是“已连接+空闲”,区别于scene的选择
    bool            executing_ = false;
    bool            hover_ = false;
    QMovie*         executingMovie_;
    QTimer*         cmdExecutionTimeoutTimer_;
    QTimer*         cmdResultIgnoreTimer_; //命令执行需要时间，过滤掉错误的状态，比如连续恢复出厂设置，恢复出厂成功的状态会在命令开始执行时就上报上来
    int             lostBroadcastCount_ = 0;
    bool            processCmdResult_;
    TransferConnectionCheckThread *connectionCheckThread_ = nullptr;

    bool transferFileIsSharedFile_ = false;
    QString transferFileName_;
    qint64 transferFilePos_ = 0;
};

#endif // COMMONDEVICEITEM_H
