#include "transferconnectioncheckthread.h"

#include <QTcpSocket>

#include "logHelper.h"
#include "commonDeviceItem.h"

TransferConnectionCheckThread::TransferConnectionCheckThread(CommonDeviceItem *device, QObject *parent)
    : QThread(parent)
    , device_(device)
{

}

void TransferConnectionCheckThread::run()
{
    int i = 0;
    do {
        if (device_->transferSocket && device_->transferSocket->state() == QAbstractSocket::ConnectedState) {
            LOG(INFO) << "TransferConnectionCheckThread transferSocket connected";
            emit transferConnected();
            break;
        } else {
            LOG(INFO) << "transferSocket is null or not connected, retry " << i;
            ++i;
            QThread::msleep(50);
        }
    } while(i < 40);
}
