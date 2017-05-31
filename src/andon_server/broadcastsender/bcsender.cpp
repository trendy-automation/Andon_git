#include <QtNetwork>
#include <QUdpSocket>
#include "bcsender.h"

#include <QByteArray>
#include <QDataStream>

//#include <QHostAddress>
//#include <QNetworkInterface>

//#define _STR(x) #x
//#define STR(X) _STR(x)


BCSender::BCSender(int interval, int port, QObject* parent) :QObject(parent)
{
    bcInterval = interval;
    bcPort = port;
}

void BCSender::run()
{
//    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
//        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
//            qDebug() << address.toString();
//    }
    timer = new QTimer(this);
    timer->setInterval(bcInterval);
    udpport=bcPort;
//    qDebug() << interval<<udpport;
    udpSocket = new QUdpSocket(this);
    messageNo = 1;
    QObject::connect(timer, &QTimer::timeout, this, &BCSender::broadcastDatagram);
    startBroadcasting();
}

void BCSender::startBroadcasting()
{
//    qDebug() << "startBroadcasting";
    timer->start();
}

void BCSender::stop()
{
    timer->stop();
    udpSocket->close();
    this->deleteLater();
}

void BCSender::broadcastDatagram()
{
    QByteArray datagram = (UDP_SRVRNAME);//. msg num " + QByteArray::number(messageNo);
    for (int i=0;i<ClientList.count();++i)
        udpSocket->writeDatagram(datagram.data(), datagram.size(),
                             ClientList.at(i), udpport);
    //qDebug() << datagram<<udpport;

    //++messageNo;
    //TODO: if > max
}

void BCSender::renewInterface(const QString &ClientIp)
{
    QByteArray datagram = "Andon client,renewInterface";
    if (ClientIp.isEmpty())
        for (int i=0;i<ClientList.count();++i)
            udpSocket->writeDatagram(datagram.data(), datagram.size(),
                                 ClientList.at(i), udpport);
    else if (!QHostAddress(ClientIp).isNull())
        udpSocket->writeDatagram(datagram.data(), datagram.size(),
                             QHostAddress(ClientIp), udpport);
    qDebug() << datagram << ClientIp;
}

void BCSender::addClient(const  QString &IP)
{
    ClientList.append(QHostAddress(IP));
    qDebug() << "UDP Sender: Client added" << IP;
}

void BCSender::clearClients()
{
    ClientList.clear();
}

void BCSender::addClients(const QString &jsonClients)
{
    qDebug()<<"BCSender started";
    for (auto row:QJsonDocument::fromJson(jsonClients.toUtf8()).array()){
        QJsonObject clientsObject=row.toObject();
        if(clientsObject.contains("IP_ADDRESS"))
            addClient(clientsObject["IP_ADDRESS"].toString());
    }
}

