#ifndef COMMONPLUGINUI_H
#define COMMONPLUGINUI_H

#include "pluginGlobal.h"

#include "devicePluginInterface.h"

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QMap>
#include <QList>
#include <QPair>
#include <QWidget>
#include <QSet>
#include <QString>

class ActivateCover;

class QVBoxLayout;
class Communicator;
class CommonClassroom;
class QHostAddress;
class QByteArray;
class QTcpSocket;

class PLUGIN_EXPORT CommonPluginUI : public DevPluginUI
{
    Q_OBJECT
public:
    explicit CommonPluginUI(const DevPluginInterface *devPlugin, QWidget *parent = nullptr);
    ~CommonPluginUI();

    void load(const QString &filePath) override;
    QString save(const QString &filePath) override;

    void activate() override;
    void deactivate() override;
    bool deviceExecuting() override;

    QSet<QString> allDevicesSNs() const;

    void checkDeviceNum();
    void setBtnsEnabled(bool enabled);
    void onClassroomViewResized(QRect rect);

    void createDialog();

    static const int kMaxDeviceNum = 40;

protected:
    void initActivateCover();

    void onShutdownClicked();
    void Shutdown(bool ok);
    void onRebootClicked();
    void Reboot(bool ok);
    void onResetClicked();
    void onUpgradeClicked();
    void onLockClicked();
    void onScreenCastClicked();
    void onScreenMonitorClicked();
    void onShareFileClicked();
    void onAddDeviceClicked();
    void onNetworkClicked();
    void onBroadcastClicked();
    void onStopBroadcastClicked();
    void onShareListChanged();

    void addDevices(const QList<QPair<QString, QString>> &sns2ids);
    void onBroadcastResponsed(const QHostAddress &ip, quint16 port, const QByteArray &responseData);
    void onNewCmdConnection(QTcpSocket *tcpSocket);
    void onNewTransferConnection(QTcpSocket *tcpSocket);
    void onBroadcastSignal();

    void onSelectionChanged();

    void resizeEvent(QResizeEvent* e) override;

protected:
    const DevPluginInterface *devPlugin_;

    Communicator    *communicator_;
    CommonClassroom *classroom_;
    QVBoxLayout     *cmdsLayout_;
    QPushButton     *addDevice2_;
    QPushButton     *lockBtn_;
    QPushButton     *shutdownBtn_;
    QPushButton     *rebootBtn_;
    QPushButton     *resetBtn_;
    QPushButton     *upgradeBtn_;
    QPushButton     *screenCastBtn_;
    QPushButton     *screenMonitorBtn_;

    QWidget         *activateCover_;
    QPushButton     *activateBtn_;
    bool            activative_;
};

#endif // COMMONPLUGINUI_H
