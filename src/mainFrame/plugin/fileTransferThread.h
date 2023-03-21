#ifndef FILETRANSFERTHREAD_H
#define FILETRANSFERTHREAD_H

#include "pluginGlobal.h"

#include <QThread>

class QTcpSocket;

#define MaxBufferSize (1024 * 1024)

class PLUGIN_EXPORT FileTransferThread : public QThread
{
    Q_OBJECT

public:
    explicit FileTransferThread(QTcpSocket *transferSocket, const QString &fileName, qint64 pos = 0, QObject *parent = nullptr);

    void run() override;

    void requestExit();

    void onBytesWritten(qint64 size);
    void onReadyRead();

signals:
    void transferProgress(qint64 val);
    void transferConnected();
    void transferDisconnected();
    void transferInterrupt();

private:
    QTcpSocket *transferSocket_;

    QString fileName_;
    qint64 pos_;

    bool exitRequest_;
    int lastPercent_;

    char data[MaxBufferSize];
};

#endif // FILETRANSFERTHREAD_H
