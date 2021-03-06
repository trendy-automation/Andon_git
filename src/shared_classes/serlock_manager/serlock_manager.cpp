#include "serlock_manager.h"
#include <QtCore/QDebug>
#include <QTextStream>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QMetaObject>
#include <QMetaProperty>

#include <QApplication>
#include "client_rpcutility.h"


//________KEEPALIVE______
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <math.h>
#define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)



const std::function<void(QVariant)> printResp = [](QVariant resp){
    for(auto i:QJsonDocument::fromJson(resp.toString().toUtf8()).array()){
        if(!i.isObject()){
            qDebug() << QJsonValue(i).toVariant() << "is not an json object";
            continue;
        }
        QStringList printList;
        for (auto o:QJsonObject(i.toObject()))
            printList << o.toString();
        qDebug() << printList.join(" ");
    }
};

//*******************************************************************************
SherlockManager::SherlockManager(QObject * parent) :
    QObject(parent),
    tcpSocket (new QTcpSocket(this)),
    connectTimer (new QTimer(this)),
    autoConnect(true),
    autoReconnect(true)
{
    qDebug() << "new Sherlock TcpClient";

    QObject::connect(connectTimer,&QTimer::timeout,this,&SherlockManager::startConnecting);
    QObject::connect(tcpSocket,&QTcpSocket::connected,connectTimer,&QTimer::stop);
    QObject::connect(this,&SherlockManager::socketDisconnected,connectTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    QObject::connect(tcpSocket, &QTcpSocket::readyRead, [=](){
        QTextStream in(tcpSocket);
        while (!in.atEnd()) {
            QString response = in.readLine();
            doReceive(response);
            emit lineReceived(response);
        }
    });

//    QObject::connect(tcpSocket, &QTcpSocket::stateChanged, this,
//                     [=](){
//        qDebug() << "Sherlock TcpClient" << tcpSocket->state();
//        });

    QObject::connect(tcpSocket,&QTcpSocket::disconnected, this, &SherlockManager::socketDisconnected);
    QObject::connect(tcpSocket,&QTcpSocket::connected,[=](){
        struct tcp_keepalive {
        u_long onoff;
        u_long keepalivetime;
        u_long keepaliveinterval;
        };
        DWORD dwError = 0L,dwBytes ;
        tcp_keepalive pClSock_tcpKeepalive={0,0,0}, sReturned = {0,0,0};
        pClSock_tcpKeepalive.onoff=1;// enable keepalive
        pClSock_tcpKeepalive.keepalivetime=TC_ALIVE_TIMEOUT; // Every KE_ALIVE_TIMEOUT ms send pack
        pClSock_tcpKeepalive.keepaliveinterval=150; // If pack does not recieved in 1.5s send again
        if (WSAIoctl(tcpSocket->socketDescriptor(), SIO_KEEPALIVE_VALS, &pClSock_tcpKeepalive,
        sizeof(pClSock_tcpKeepalive), &sReturned, sizeof(sReturned), &dwBytes,
        NULL, NULL) != 0)
        {dwError = WSAGetLastError() ;
        qWarning((char*)dwError); }
        qDebug()<< QString("Sherlock TcpClient connected to %1:%2").arg(deviceIp).arg(port);
        emit this->socketConnected();
    });

//    protocolMap.insert("RD",[this](QStringList responseList){
//    });
    //if(autoStart)
    QObject::connect(this,&SherlockManager::socketConnected, this, &SherlockManager::VisuonRun);
    if(autoConnect)
        startConnecting();

    QObject::connect(this,&SherlockManager::visionResult,[=](const QString &result){

            ClientRpcUtility*serverRpc=qApp->findChild<ClientRpcUtility*>("serverRpc");
            if(serverRpc)
                serverRpc->Query2Json(QString("SELECT PART_NAME, DEVICE_NAME, ERROR "
                                              "FROM PRODUCTION_VERSION_PRODUSED(%1)")
                                                     .arg(result), printResp);
    });

}

void SherlockManager::VisuonRun()
{
    if(!this->isSocketConnected())
        return;
    qDebug()<< "SherlockManager::VisuonRun";
//    for(auto &vision:this->findChildren<QStateMachine*>()){
//        qDebug()<<"this->findChildren<QStateMachine*>()"<<vision;
//        vision->deleteLater();
//    }
    /*QStateMachine **/ vision = new QStateMachine();
    QState      *INIT = new QState(vision);
    QState      *INITOK = new QState(vision);
//    QState      *PROGRAMM = new QState(vision); //MEDALION
    QState      *INSPECTING = new QState(vision);
//    QState      *SIDE_RH = new QState(vision); //SIDE_LH
//    QState      *ER = new QState(vision);
//    QState      *RESULT = new QState(vision);
//    QState      *RESULT_NOK = new QState(vision);
    QFinalState *FINISH = new QFinalState(vision);
// qDebug()<<"INIT"<<INIT
//         <<"INITOK"<<INITOK
//         <<"INSPECTING"<<INSPECTING
// //         <<"RESULT_NOK"<<RESULT_NOK
//         <<"FINISH"<<FINISH;

    /*QTimer **/initTimer = new QTimer(vision);

    vision->setInitialState(INIT);

//    QObject::connect(this, &SherlockManager::VisionStop, vision, &QStateMachine::stop);
//    QObject::connect(tcpSocket,  &QTcpSocket::disconnected, vision, &QStateMachine::deleteLater);
    QObject::connect(tcpSocket,  &QTcpSocket::disconnected, [this  /*,vision,initTimer,INIT,INITOK,INSPECTING,FINISH*/ ](){

//        //INIT->removeTransition();
//        INIT->disconnect();
//        INITOK->disconnect();
//        INSPECTING->disconnect();
//        FINISH->disconnect();
//        INIT->deleteLater();
//        INITOK->deleteLater();
//        INSPECTING->deleteLater();
//        FINISH->deleteLater();
//        initTimer->disconnect();
        initTimer->stop();
        initTimer->deleteLater();
//        vision->disconnect();
        vision->stop();
        vision->deleteLater();
    });
    QObject::connect(vision, &QStateMachine::stopped, vision, &QStateMachine::deleteLater);
    QObject::connect(vision, &QStateMachine::finished, vision, &QStateMachine::start);
//    QObject::connect(vision, &QStateMachine::finished, vision, &QStateMachine::deleteLater);
    QObject::connect(FINISH, &QFinalState::entered, [this/*,vision*/]() {
             qDebug()<<"FINISH. Res ="<<vision->property("result").toString();
        });
//    QObject::connect(RESULT_NOK, &QState::entered, [this]() {
//             qDebug()<<"RESULT_NOK. Res ="<<vision->property("result").toString();
//        });
//    QObject::connect(vision, &QStateMachine::destroyed, initTimer, &QTimer::deleteLater);
//    QObject::connect(tcpSocket,  &QTcpSocket::disconnected, initTimer, &QTimer::deleteLater);
    QObject::connect(this, &SherlockManager::lineReceived, [this/*,vision,initTimer*/](const QString &result){
        if(!vision->configuration().values().isEmpty())
            vision->setProperty("lastState",qVariantFromValue((void*)vision->configuration().values().first()));
//        qDebug()<<"lastState:"<<vision->property("lastState").value<void*>();
       QStringList messege = result.split(":");
       QString mes = messege.at(0);
       messege.removeFirst();
        if(mes.compare("INITOK")==0) {
//            qDebug()<<"INITOK. vision->configuration()" << vision->configuration();
            emit visionInitOk();
//            qDebug()<<"emit visionInitOk() vision->configuration()" << vision->configuration();
        }
        if(mes.compare("INSPECTING_ON")==0) {
//            qDebug()<<"INSPECTING_ON. vision->configuration()" << vision->configuration();
            emit visionInspecting();
//            qDebug()<<"emit visionInspecting() vision->configuration()" << vision->configuration();
        }
        if(mes.compare("RESULT_NOK")==0) {
            vision->setProperty("result","NOK");
            emit visionResultNok();
//            qDebug()<<"RESULT_NOK. vision->configuration()" << vision->configuration();
        }
        //if(mes.compare("INSPECTING_OFF")==0) {vision->setProperty("result",result); emit visionResult(result);}
        if(mes.compare("RESULT")==0) {
            QString res= messege.join("");
            vision->setProperty("result",res);
//            qDebug()<<"Medalion version:"<<res;
            emit visionResult(res);
        }
        if(mes.compare("ER")==0) {
            vision->setProperty("result",result);
            emit visionError(result);
        }
        initTimer->start(5000);
    });

    INIT->addTransition(this, &SherlockManager::visionInitOk, INITOK);
    INITOK->addTransition(this, &SherlockManager::visionInspecting, INSPECTING);
    INSPECTING->addTransition(this, &SherlockManager::visionResult, FINISH);
    INSPECTING->addTransition(this, &SherlockManager::visionError, FINISH);
    INSPECTING->addTransition(this, &SherlockManager::visionResultNok, FINISH);
    INIT->addTransition(initTimer, &QTimer::timeout, INIT);
    INITOK->addTransition(initTimer, &QTimer::timeout, INIT);
    INSPECTING->addTransition(initTimer, &QTimer::timeout, INIT);
    INIT->addTransition(this, &SherlockManager::sendFailed, FINISH);
    INITOK->addTransition(this, &SherlockManager::sendFailed, FINISH);
    INSPECTING->addTransition(this, &SherlockManager::sendFailed, FINISH);


//    ER->addTransition(this, &SherlockManager::, FINISH);
//    RESULT->addTransition(this, &SherlockManager::, FINISH);

    //QObject::connect(vision, &QStateMachine::stopped, initTimer, &QTimer::deleteLater);
    QObject::connect(INIT, &QState::entered, [this]() {Send("INIT");}); //initTimer->start(5000);
    QObject::connect(INITOK, &QState::entered, [this]() {Send("EXEC");});
//    QObject::connect(INSPECTING, &QState::entered, [this]() {});

    vision->start();
}

SherlockManager::~SherlockManager()
{
    if (isSocketConnected())
        tcpSocket->disconnect();
}

void SherlockManager::startConnecting()
{
    if(!deviceIp.isEmpty() && (port!=0)
            && tcpSocket->state()!=QAbstractSocket::ConnectingState
            ) {
        tcpSocket->connectToHost(deviceIp,port);
        if (autoReconnect) {
            //qDebug()<<"startTimer Sherlock TcpClient";
            connectTimer->start(TC_RECONNECT_TIMEOUT);
        }
    }
}

void SherlockManager::Send(QString Data)
{
    if (tcpSocket->isOpen())
        tcpSocket->write(Data.toLatin1().append("\n"));
    else
        emit sendFailed(Data);
}

void SherlockManager::doReceive(const QString &response)
{
//    qDebug() << "Sherlock TcpClient" << "Sherlock response:" << response;
//    QStringList  responseList=response.split(",");
//    return responseProcessingMap.contains(responseList.at(0));
//    bool ok=true;
//    switch(messageList.indexOf(responseList[0])){
//    case 0:
//        break;
//    case 1:
//        break;
//    case 2:
//        break;
//    default:
//        qDebug() << "Sherlock TcpClient" << "unknown TC response:" << response;
//        break;
//    }
//    if (!ok) {qDebug() << "Sherlock TcpClient" << "Bad TC response:" << response;
//    }
}

void SherlockManager::VisionStop()
{

}

void SherlockManager::setAuxProperties(const QString &auxProperties)
{
    QVariantMap objProperties = QJsonDocument::fromJson(auxProperties.toUtf8()).object().toVariantMap();
    const QMetaObject *metaObj = this->metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i)
        if (objProperties.contains(metaObj->property(i).name()))
            metaObj->property(i).write(this,objProperties.value(metaObj->property(i).name()));
}
