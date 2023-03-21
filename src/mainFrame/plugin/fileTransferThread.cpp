#include "fileTransferThread.h"

#include "logHelper.h"

#include <QTcpSocket>
#include <QFile>

#include <QHostAddress>
#include <QString>

FileTransferThread::FileTransferThread(QTcpSocket *transferSocket, const QString &fileName, qint64 pos, QObject *parent)
    : QThread(parent)
    , transferSocket_(transferSocket)
    , fileName_(fileName)
    , pos_(pos)
    , exitRequest_(false)
    , lastPercent_(0)
{
    transferSocket_->setParent(nullptr);
}

void FileTransferThread::run()
{
    LOG(INFO) << "FileTransferThread::run";
    transferSocket_->moveToThread(this);

    connect(transferSocket_, &QTcpSocket::connected, this, &FileTransferThread::transferConnected);
    connect(transferSocket_, &QTcpSocket::disconnected, this, &FileTransferThread::transferDisconnected);
    connect(transferSocket_, &QTcpSocket::destroyed, this, &FileTransferThread::transferDisconnected);
    connect(transferSocket_, &QTcpSocket::bytesWritten, this, &FileTransferThread::onBytesWritten);
    connect(transferSocket_, &QTcpSocket::readyRead, this, &FileTransferThread::onReadyRead);

    QFile file(fileName_);
    qint64 transferedSize = 0;
    if (file.open(QIODevice::ReadOnly)) {
        file.seek(pos_);
        LOG(INFO) << "sending data to " << transferSocket_->peerAddress().toString().toStdString()
                  << ":" << transferSocket_->peerPort();
//        const int MaxBufferSize = 1024 * 1024;
//        char data[MaxBufferSize] = { 0 };
        do {
//            sleep(1);
            if (exitRequest_) {
                LOG(INFO) << "FileTransferThread exitRequest_=true";
                LOG(INFO) << "exit request";
                break;
            }
            memset(data, 0, MaxBufferSize);
            qint64 res = file.read(data, MaxBufferSize);
            if (res == 0) {
                LOG(INFO) << "end of file";
                break;
            } else if (res == -1) {
                LOG(ERROR) << "error happens while reading";
                emit transferInterrupt();
                break;
            } else {
//                LOG(INFO) << "... read " << res << " bytes";
            }

            res = transferSocket_->write(data, res);
            if (transferSocket_->waitForBytesWritten()) {
                if (res == 0) {
                    LOG(INFO) << "!!!!! send no data";
                    emit transferInterrupt();
                    break;
                } else if (res == -1) {
                    LOG(ERROR) << "error happens while sending";
                    emit transferInterrupt();
                    break;
                } else {
                    transferedSize += res;
    //                transferSocket_->flush();
//                    LOG(INFO) << "... send " << res << " bytes";
//                    LOG(INFO) << "transfered " << transferedSize << "(total:"<< file.size() << ")bytes progress:" << (transferedSize + pos_) * 100 / file.size();
                    qint64 percent = (transferedSize + pos_) * 100 / file.size();
                    if (lastPercent_ != percent) {
                        lastPercent_ = percent;
                        emit transferProgress(percent);
                    }
                }
            } else {
                LOG(INFO) << "transferSocket_->waitForBytesWritten failed";
                emit transferInterrupt();
                break;
            }
        } while (transferSocket_ && transferSocket_->state() == QAbstractSocket::ConnectedState);
        file.close();
    }

    if (transferSocket_) {
        LOG(INFO) << "close transferSocket_";
        transferSocket_->close();
        transferSocket_->deleteLater();
        transferSocket_ = nullptr;
    }
    LOG(INFO) << "FileTransferThread::run finished";
}

void FileTransferThread::requestExit()
{
    LOG(INFO) << "FileTransferThread::requestExit";
    exitRequest_ = true;
}

void FileTransferThread::onBytesWritten(qint64 size)
{
//    LOG(INFO) << size << " bytes written";
}

void FileTransferThread::onReadyRead()
{
    if (transferSocket_ && transferSocket_->state() == QAbstractSocket::ConnectedState) {
        QByteArray readData = transferSocket_->readAll();
        LOG(INFO) << "read " << readData.size() << " bytes";
    }
}
