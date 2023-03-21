#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "pluginGlobal.h"

#include <QObject>
#include <QString>
#include <QMap>
#include <QHostAddress>
#include <QThread>

class QUdpSocket;
class QTimer;
class QTcpServer;
class QTcpSocket;

class PLUGIN_EXPORT Communicator : public QObject
{
    Q_OBJECT

public:
    Communicator(quint16 broadcastPort, quint16 cmdPort, quint16 transferPort, QObject *parent = nullptr);

    static Communicator* Communicator::instance();

    void startBroadcast(const QByteArray &datagram);
    void stopBroadcast();
    void udpMsg(const QHostAddress &ip, quint16 port, const QByteArray &data);

private:
    void broadcastData();
    void processBroadcastResponse();
    void onNewCmdConnection();
    void onNewTransferConnection();

signals:
    void broadcastResponsed(QHostAddress ip, quint16 port, QByteArray data);
    void newCmdConnection(QTcpSocket *tcpSocket);
    void newTransferConnection(QTcpSocket *tcpSocket);
    void broadcastSignal();

private:
    static Communicator *instance_;

    int broadcastNo_;
    QUdpSocket *broadcastor_;
    QByteArray broadcastData_;
    QTimer *broadcastTimer_;

    QTcpServer *cmdServer_;
    QTcpServer *transferServer_;
};

#endif // COMMUNICATOR_H
