#ifndef DEVICEDETAIL_H
#define DEVICEDETAIL_H

#include <QWidget>

class DevPluginInterface;

namespace Ui {
class DeviceDetail;
}

class DeviceDetail : public QWidget
{
    Q_OBJECT

public:
    DeviceDetail(const DevPluginInterface *plugin,
        const QString& sn,
        const QString& name,
        const QString& version,
        int power,
        const QString& powerImage,
        bool connected,
        QWidget *parent = nullptr);
    ~DeviceDetail();

signals:
    void deviceNameEdited(const QString&);

private:
    Ui::DeviceDetail *ui;

    const DevPluginInterface *plugin_;
    bool editing_;
    QString name_;
};

#endif // DEVICEDETAIL_H
