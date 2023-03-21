#ifndef TRANSFERCONNECTIONCHECKTHREAD_H
#define TRANSFERCONNECTIONCHECKTHREAD_H

#include <QThread>

class CommonDeviceItem;

class TransferConnectionCheckThread : public QThread
{
    Q_OBJECT
public:
    explicit TransferConnectionCheckThread(CommonDeviceItem *device, QObject *parent = nullptr);

    void run() override;

signals:
    void transferConnected();

private:
    CommonDeviceItem *device_;
};

#endif // TRANSFERCONNECTIONCHECKTHREAD_H
