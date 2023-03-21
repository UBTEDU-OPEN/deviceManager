#ifndef COMMONCLASSROOM_H
#define COMMONCLASSROOM_H

#include "pluginGlobal.h"

#include <QFrame>
#include <QMap>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

#include <QKeyEvent>

class QGraphicsView;
class GraphicsScene;
class CommonDeviceItem;
class BackgroundWidget;
class DevPluginInterface;

class PLUGIN_EXPORT CommonClassroom : public QFrame
{
    Q_OBJECT

public:
    static const int windowFixedWidth = 1500;
    static const int windowFixedHeight = 769;

public:
    explicit CommonClassroom(const DevPluginInterface *plugin, QWidget *parent = nullptr);

    void load(const QJsonObject &classroomData);
    void save(QJsonObject &classroomData) const;

    void addDevices(const QList<QPair<QString, QString>> &sns2ids);
    void delDevice(const QString& sn);
    QMap<QString, CommonDeviceItem*> deviceItems() const;
    QMap<QString, CommonDeviceItem*> selectedDeviceItems() const;
    QMap<QString, CommonDeviceItem*> sceneSelectedDeviceItems() const;
    QMap<QString, CommonDeviceItem*> connectedDeviceItems() const;
    static QMap<CommonDeviceItem*,QRectF> deviceMap_;
    bool selectedDevicesAllLocked() const;

    void addDeviceItem(const QString& sn,
                       const QString& id,
                       double x,
                       double y,
                       uint32_t status);
    void checkDevicesNum();

    void shutdownDevices();
    void rebootDevices();
    void resetDevices(bool accepted, int saveUserData);
    void lockDevices(bool targetLockState);

    void setSelectAllChecked(bool);
    void sortDevices();

    bool positionLocked() { return editLocked_; }
    void setPositionLocked(bool locked);

public slots:
    void onPositionLockClicked();

signals:
    void deviceDeleted();
    void selectionChanged();
    void addBtnClicked();

protected:
    void resizeEvent(QResizeEvent* e) override;
    
protected:
    const DevPluginInterface *plugin_;

    QGraphicsView  *view_;
    GraphicsScene  *scene_;
    QCheckBox      *selectAll_;
    QLabel         *selectionNote_;
    QPushButton    *positionLock_;
    bool           editLocked_ = true;
    bool           boxChoose_ = false;
    QMap<QString,CommonDeviceItem*> resetDevices_;
    int            resetDeviceCount_;
    int            resetSuccCount_;
    int            resetFailCount_;
    BackgroundWidget *backgroundWidget_;
};

#endif // COMMONCLASSROOM_H
