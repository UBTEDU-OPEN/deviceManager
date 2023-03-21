#include "communicator.h"

#include "logHelper.h"
#include "md5.h"

#include <QUdpSocket>
#include <QTimer>
#include <QByteArray>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>

#define BROADCAST_PORT 45454
#define CMD_PORT 8989
#define TRANSFER_PORT 9090

Communicator *Communicator::instance_ = nullptr;

Communicator::Communicator(quint16 broadcastPort, quint16 cmdPort, quint16 transferPort, QObject *parent)
    : QObject(parent)
    , broadcastNo_(0)
{
    broadcastor_ = new QUdpSocket(this);
    broadcastor_->bind(broadcastPort, QUdpSocket::ShareAddress);
    broadcastTimer_ = new QTimer(this);
    connect(broadcastTimer_, &QTimer::timeout, this, &Communicator::broadcastData);
    connect(broadcastor_, &QUdpSocket::readyRead, this, &Communicator::processBroadcastResponse);

    cmdServer_ = new QTcpServer(this);
    if (!cmdServer_->listen(QHostAddress::Any, cmdPort)) {
        LOG(ERROR) << "!!!!!command listener failed";
    }
    connect(cmdServer_, &QTcpServer::newConnection, this, &Communicator::onNewCmdConnection);

    transferServer_ = new QTcpServer(this);
    if (!transferServer_->listen(QHostAddress::Any, transferPort)) {
        LOG(ERROR) << "!!!!!transfer listener failed";
    }
    connect(transferServer_, &QTcpServer::newConnection, this, &Communicator::onNewTransferConnection);
}

Communicator *Communicator::Communicator::instance()
{
    if (!instance_) {
        instance_ = new Communicator(BROADCAST_PORT, CMD_PORT, TRANSFER_PORT);
    }
    return instance_;
}

void Communicator::startBroadcast(const QByteArray &datagram)
{
    broadcastTimer_->stop();
    broadcastData_ = datagram;
    broadcastData();
    broadcastTimer_->start(2500);
}

void Communicator::stopBroadcast()
{
    broadcastTimer_->stop();
}

void Communicator::udpMsg(const QHostAddress &ip, quint16 port, const QByteArray &data)
{
    broadcastor_->writeDatagram(data, ip, port);
}

void Communicator::broadcastData()
{
//    LOG(INFO) << "Broadcast datagram:" << broadcastData_.constData() << " this:" << reinterpret_cast<int>(this);
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    for (QNetworkInterface interface : interfaceList) {
//        LOG(INFO) << "network type:" << interface.type();
        if (QNetworkInterface::InterfaceType::Loopback == interface.type()) { //过滤aibox USB网络
            continue;
        }
//        LOG(INFO) << "network interface:" << interface.humanReadableName().toStdString();
        QList<QNetworkAddressEntry> entryList = interface.addressEntries();
        for (QNetworkAddressEntry entry : entryList) {
            QString str = entry.broadcast().toString();
            if (str != "") {
//                LOG(INFO) << "\tbroadcast ip:" << str.toStdString();
                broadcastor_->writeDatagram(broadcastData_, QHostAddress(str), broadcastor_->localPort());
            }
        }
    }
    emit broadcastSignal();
    ++broadcastNo_;
}

void Communicator::processBroadcastResponse()
{
    QByteArray datagram;
    while (broadcastor_->hasPendingDatagrams()) {
        datagram.resize(int(broadcastor_->pendingDatagramSize()));
        QHostAddress host;
        quint16 port;
        broadcastor_->readDatagram(datagram.data(), datagram.size(), &host, &port);
//        LOG(INFO) << "Received datagram:" << datagram.constData() << " from:" << host.toString().toStdString() << " port:" << port;
        emit broadcastResponsed(host, port, datagram);
    }
}

void Communicator::onNewCmdConnection()
{
    while (cmdServer_->hasPendingConnections()) {
        auto tcpSocket = cmdServer_->nextPendingConnection();
        emit newCmdConnection(tcpSocket);
    }
}

void Communicator::onNewTransferConnection()
{
    while (transferServer_->hasPendingConnections()) {
        auto tcpSocket = transferServer_->nextPendingConnection();
        emit newTransferConnection(tcpSocket);
    }
}
