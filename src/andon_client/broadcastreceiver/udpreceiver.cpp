#include <QtNetwork>
#include <QUdpSocket>
#include <QDebug>

#include <QByteArray>
#include <QDataStream>

#include <QHostAddress>
#include <QNetworkInterface>
#include <QTimer>
#include "udpreceiver.h"

//#define _STR(x) #x
//#define STR(X) _STR(x)


UdpReceiver::UdpReceiver(QObject *parent) :
    QObject(parent)
{
    isserverfound = false;
    udpSocket = new QUdpSocket(this);
    //qDebug() << "udpSocket connecting state " << udpSocket->state();
    //QNativeSocketEngine::bind() was not called in QAbstractSocket::UnconnectedState
    //TODO make one timer
//    if(udpSocket->state()!=QAbstractSocket::UnconnectedState){
        if(!udpSocket->bind(UDP_PORT, QUdpSocket::ShareAddress)) {
            QTimer *udpTimer = new QTimer;
            QObject::connect(udpTimer,&QTimer::timeout,[this,udpTimer](){
                //qDebug() <<"udpSocket port " << UDP_PORT << " connecting state " << udpSocket->state();
                //if(udpSocket->state()!=QAbstractSocket::UnconnectedState){
                    if (udpSocket->bind(UDP_PORT, QUdpSocket::ShareAddress)) {
                        udpTimer->stop();
                        udpTimer->deleteLater();
                    }
                //}
            });
            udpTimer->start(1000);
        }
//    } else {
//        QTimer *udpTimer = new QTimer;
//        QObject::connect(udpTimer,&QTimer::timeout,[this](){
//            //qDebug() <<"udpSocket port " << UDP_PORT << " connecting state " << udpSocket->state();
//            if(udpSocket->state()!=QAbstractSocket::UnconnectedState){
//                if (!udpSocket->bind(UDP_PORT, QUdpSocket::ShareAddress)) {
//                    udpTimer->stop();
//                    udpTimer->deleteLater();
//                }
//            }
//        });
//        udpTimer->start(1000);
//    }


    SvrHost="0.0.0.0";

    QObject::connect(udpSocket, &QUdpSocket::readyRead, this, &UdpReceiver::processPendingDatagrams);

    //TODO: QUdpSocket::readyRead emit every time if new broadcast

}

void UdpReceiver::start()
{
    isserverfound = false;
}
void UdpReceiver::stop()
{
    isserverfound = true;
}

void UdpReceiver::processPendingDatagrams()
{
    //! [2]
    // isserverfound = false;
    //while(socketopen){
        if (udpSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(udpSocket->pendingDatagramSize());
            QHostAddress ServerAddress;
            udpSocket->readDatagram(datagram.data(), datagram.size(), &ServerAddress);
//            qDebug() << "ServerAddress.toString()"<<ServerAddress.toString();
            if (ServerAddress.toString().length()>0) {
                if (!isserverfound) {
                if (QString(QString(QByteArray::fromRawData(datagram.data(),datagram.size()))).contains((UDP_SRVRNAME))){
//                    qDebug() << "Andon server found on IP: " << ServerAddress;
                    //this->disconnect(udpSocket, &QUdpSocket::readyRead, this, &UdpReceiver::processPendingDatagrams);
//                    udpSocket->close();
//                    socketopen=false;
                    isserverfound=true;
                    ServerAddress = QHostAddress(ServerAddress.toIPv4Address());
                    qDebug()<<"ServerAddress"<<ServerAddress.toString();
                    SvrHost = ServerAddress;
                    emit serverfound(ServerAddress);
//                    QTimer * diagramtimer =new QTimer;
//                    diagramtimer->setInterval(10000);
//                    QObject::connect(diagramtimer,&QTimer::timeout,[this](){
//                        QObject::connect(udpSocket, &QUdpSocket::readyRead, this, &UdpReceiver::processPendingDatagrams);
//                });
                }
                }
                if (QString(QString(QByteArray::fromRawData(datagram.data(),datagram.size()))).contains("Andon client,renewInterface")){
                    //qDebug() << "Andon client,renewInterface";
                    emit renewInterface(SvrHost);
                }
            }
        }
    //}
    //! [2]
}


