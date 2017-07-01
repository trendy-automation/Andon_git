#include "ketcp_object.h"

#include <QtCore/QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaObject>
#include <QMetaProperty>

#include <QApplication>

#include "client_rpcutility.h"


//________KEEPALIVE______
//#include <sys/socket.h>
//#include <netinet/tcp.h>
//#include <netinet/in.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <math.h>
#define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)


static void printResp (QVariant resp){
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
}

//*******************************************************************************
KeTcpObject::KeTcpObject(QObject *parent) :
    QObject(parent),
    tcpSocket (new QTcpSocket(this)),
    deviceIp(QString()),
    port(0),
    pass(QString())  
    ,KeSysStart(QDateTime::fromMSecsSinceEpoch(0))
{

//    ConnectionValid=false;
//    ConnectionIsOk=false;
    AutoInputs=true;
    AutoRefresh=false;
    AutoEvent=true;
    AutoReconnect=true;
    initIO();
//    QObject::connect(this, &KeTcpObject::connected, this, &KeTcpObject::readInputs); //todo read IO
    QObject::connect(this, &KeTcpObject::sysTimeWrited, [=](){enableEvents();});
    QObject::connect(this, &KeTcpObject::sysTimeWrited,[=](){enableRefreshing();});
    QObject::connect(this, &KeTcpObject::sysTimeBad, this, &KeTcpObject::refreshSysTime);

    QObject::connect(tcpSocket, &QTcpSocket::readyRead, [=](){
        QTextStream in(tcpSocket);
        while (!in.atEnd()) {
            QString response = in.readLine();
            actResps(response);
            emit DataReceived(response);
        }
    });

    QObject::connect(tcpSocket, &QTcpSocket::stateChanged, this,
                     [=](){
//        qDebug() << getDeviceName() << tcpSocket->state();
        });

    QObject::connect(tcpSocket,  &QTcpSocket::disconnected, this,
                     [=](){
        //TODO launch this lymbda then start.
        emit this->disconnected();
        KeSysStart=QDateTime::fromMSecsSinceEpoch(0);
        //TODO all properties - undentify
        initIO();

        if (AutoReconnect)
            startConnecting();
    });

    sysTimeTimer.setInterval(2000);

    connectTimer.setInterval(KE_RECONNECT_TIMEOUT);
    QObject::connect(&connectTimer,&QTimer::timeout,[=](){emit startConnecting(false);});

    QObject::connect(tcpSocket,&QTcpSocket::connected,[=](){
        connectTimer.stop();
        struct tcp_keepalive {
        u_long onoff;
        u_long keepalivetime;
        u_long keepaliveinterval;
        };

        DWORD dwError = 0L,dwBytes ;
        tcp_keepalive pClSock_tcpKeepalive={0,0,0}, sReturned = {0,0,0};
        pClSock_tcpKeepalive.onoff=1; // enable keepalive
        pClSock_tcpKeepalive.keepalivetime=KE_ALIVE_TIMEOUT; // Every KE_ALIVE_TIMEOUT ms send pack
        pClSock_tcpKeepalive.keepaliveinterval=150; // If pack does not recieved in 1.5s send again
 
        if (WSAIoctl(tcpSocket->socketDescriptor(), SIO_KEEPALIVE_VALS, &pClSock_tcpKeepalive,
        sizeof(pClSock_tcpKeepalive), &sReturned, sizeof(sReturned), &dwBytes,
        NULL, NULL) != 0)
        {dwError = WSAGetLastError() ;
        qWarning((char*)dwError); }

        keSend(QByteArray("$KE,PSW,SET,").append(pass));
        qDebug()<< getDeviceName() << "Login to KBX100" << deviceIp << ":" << port <<"pass"<<pass;
        if (!AutoEvent)
            keSend("$KE,EVT,OFF");
        if (AutoRefresh)
            keSend("$KE,DAT,ON");
        if (AutoInputs)
            readInputs();
        if (AutoEvent)
            refreshSysTime();
        keSend("$KE"); //TODO remake the connect signal
//        qDebug()<<"Tcp socket connected lambda finish";
    });

    QObject::connect(this,&KeTcpObject::inputChanged,[=](int InputNum, bool InputVal){
        emit IOEvent(QString("input_%1").arg(InputNum), InputVal);
    });

    QObject::connect(this,&KeTcpObject::inputCodeChanged,[=](quint32 inputCode){
        emit IOEvent("inputCode", inputCode);
    });

    QObject::connect(this,&KeTcpObject::IOEvent,[=](const QString &ioName,const QVariant &val){
        if(ioName=="inputCode" && val!=0){
            ClientRpcUtility*serverRpc=qApp->findChild<ClientRpcUtility*>("serverRpc");
            if(serverRpc)
                serverRpc->Query2Json(QString("SELECT DISTINCT DEVICE_NAME, "
                                              "MOLD_NAME FROM PRODUCTION_PART_PRODUSED (%1,%2,0,'%3')")
                                                     .arg(val.toInt()).arg(idDevice)
                                                     .arg(QDateTime::currentDateTime().toString("dd.MM.yy HH:mm:ss")),
                                                     static_cast<std::function<void(QVariant)>>(printResp));
        }
    });

    responseProcessingMap.insert("RD",[this](QStringList responseList){
        if (responseList.at(1)=="ALL") {
            int inCnt = responseList.at(2).length();
            if(inCnt!=inputs.count() || inputs.isEmpty())
                inputs.resize(inCnt);
            for (int i=0;i<inCnt;++i) {
                bool inputState=false;
                switch (responseList.at(2).at(i).toLatin1()) {
                case 49:  //1
                    inputState=true;
                case 48:  //0
                    if (inputs.at(i)!=inputState) {
                        inputs.setBit(i,inputState);
                        emit inputsChanged(inputs);
                    }
                    break;
                case 120: //x
                    inputs.clearBit(i);
                    break;
                }
            }
        }
    });
    watchCodesStart(); //TODO conditions for code start, disconnect lymbda after
}

KeTcpObject::~KeTcpObject()
{
    if (isSocketConnected())
        tcpSocket->disconnect();
}

void KeTcpObject::startConnecting(bool startTimer)
{
    if(!deviceIp.isEmpty() && !pass.isEmpty() && (port!=0) &&
            tcpSocket->state()!=QAbstractSocket::ConnectingState) {
        tcpSocket->connectToHost(deviceIp,port);
        if (startTimer) {
            qDebug()<<getDeviceName();
            connectTimer.start();
        }
    }
}

bool KeTcpObject::keSend(QString replay)
{
    bool sockOpn=tcpSocket->isOpen();
    if (sockOpn)
        tcpSocket->write(replay.toLatin1().append("\r\n"));
    else
        qDebug() << getDeviceName() << "KE tcpSocket not opened. Cannot send" << replay;
    return sockOpn;
}

void KeTcpObject::actResps(QString response)
{
//    qDebug() << this->getDeviceName() << "KE response:" << response;
    QStringList  responseList=response.split(",");

//    return responseProcessingMap.contains(responseList.at(0));



    bool ok=true;
    switch(KeList.indexOf(responseList[0])){
    case 0:
        // Login OK/NOK
        if (responseList.at(2)=="OK") {
            emit accessAllowed();
        } else
            qDebug() << getDeviceName() << "Bad KE password:" << pass;
        break;
    case 1:
        // OK
        emit connected();
        break;
    case 2:
        // ERR
        qDebug() << getDeviceName() << "Wrong KE syntax!";
        break;
    case 3:
        // DAT OK?
        if (responseList.at(1)=="OK")
            qDebug() << getDeviceName() << "DAT streaming on!";
        break;
    case 4:
        // WR OK?
        //        if (responseList.at(1)=="OK") ;
        //qDebug() << "Writing ok!";
        break;
    case 5:
        // WRA
        if (responseList.at(1)=="OK") emit OutputsWriten(responseList.at(2).toInt(&ok,10));
        break;
    case 6:
        // REL
        if (responseList.at(1)=="OK") emit RelayWriten();
        break;
    case 7:
        //SYSTEM TIME dec
        setSysTime(responseList.at(1).toInt(&ok));
        break;
    case 8:
        //RD inputs 1-6
    {
            int inCnt = responseList.at(1).size();
            bool firstReading=false;
//            if(inCnt!=inputs.count() || inputs.isEmpty()) {
            if(inputs.isEmpty()) {
                inputs.resize(inCnt);
                firstReading=true;
            }
            for (int i=0;i<inCnt;++i) {
                if (inputs.testBit(i)!=(responseList.at(1).at(i).digitValue()==1)) {
                    inputs.toggleBit(i);
                    emit inputsChanged(inputs);
                }
            }
            if (firstReading)
                emitIfReady();
    }
        break;
    case 9:
        //RID output 1-12
        if (responseList.at(1)=="ALL"){
            if(outputs.isEmpty())
                outputs.resize(responseList.at(2).size());
            for (int i=0;i<responseList.at(2).size()-1;++i)
                outputs[i]=responseList.at(2).at(i).digitValue()==1;
        }
        break;
    case 10:
        //RDR relay 1-4
        if (responseList.at(1)=="ALL"){
            if(relays.isEmpty())
                relays.resize(responseList.at(2).size());
            for (int i=0;i<responseList.at(2).size()-1;++i)
                relays[i]=responseList.at(2).at(i).digitValue()==1;
        }
        break;
    case 11:
        //ADC dec
        qDebug() << getDeviceName() << "ADC dec!" << responseList.at(1).toInt(&ok) << responseList.at(2).toFloat(&ok);
        int channel;
        channel = responseList.at(1).toInt(&ok);
        float value;
        value = responseList.at(2).toFloat(&ok);
        if (ok) {
            if (Adc[channel]!=value) {
                Adc[channel]=value;
                emit AdcChange(channel, value);
            }
        }
        break;
    case 12:
        //TMP temperature float
        tmp = responseList.at(1).toFloat(&ok);
        if((abs(temperatur)>(abs(tmp)+tolerance)) || (abs(temperatur)<abs(abs(tmp)-tolerance))){
            temperature=tmp;
            temperatur=tmp;
            emit tempChanged(temperatur);
        }
        //qDebug()<<"temperatur"<<temperatur;
        break;
    case 13:
        //IMPL,1 counter 1 dec
        counter[responseList.at(1).toInt(&ok,10)]= responseList.at(5).toInt(&ok,10)*32766 + responseList.at(6).toInt(&ok,10);
        break;
    case 14:
        //INT,1 counter 1 dec
        setSysTime(responseList.at(3).toInt(&ok));
        break;
    case 15:
        //EVT
            if (responseList.at(1)=="IN") {
                int inputNumber  = responseList.at(3).toInt(&ok)-1;
                bool inputState = responseList.at(4).toInt(&ok);
                if (ok) {
                    if (!inputs.isEmpty())
                        inputs.setBit(inputNumber,inputState);
                    int deltaTime=QDateTime::currentDateTime().secsTo(KeSysStart.addSecs(responseList.at(2).toInt(&ok)));
                    if (KeSysStart==QDateTime::fromMSecsSinceEpoch(0) || abs(deltaTime)>5)
                        refreshSysTime();
                    if (sysTimeTimer.isActive())
                        emit inputsWasChanged(inputNumber,inputState, deltaTime);
                    else {
                        emit inputsChanged(inputs);
                        if(abs(deltaTime)>2)
                            setSysTime(responseList.at(2).toInt(&ok));
                    }
                }
            }
        break;
    case 16:
        //SLINF ?
        qDebug() << getDeviceName() << "Was Events";
        emit wasEvents();
        break;
    case 17:
        //#DATA_SAVE
        break;
    case 18:
        //#FLAGS
        break;
    case 19:
        //JConfig from FLASH
        break;
    case 20:
        //#
        break;
    case 21:
        //#EVT#EVT
        break;
    default:
        //TODO: advanced KE switch case
        qDebug() << getDeviceName() << "unknown KE response:" << response;
//        ok=false;
//        return 0;
        break;
    }
    if (!ok) {qDebug() << getDeviceName() << "Bad KE response:" << response;
//        return 0;
    }
//    else
//        return KeList.indexOf(responseList[0])+1;
    //emitIfReady();
}
//*******************************************************************************

void KeTcpObject::setSysTime(int secs)
{
//    QStringList resList={"writed","updated","rewrited"}; //System time
    int res;
    QDateTime newTime = QDateTime::currentDateTime().addSecs(-secs);
    if (KeSysStart==QDateTime::fromMSecsSinceEpoch(0)) {
        res = 0;
        emit sysTimeWrited();
    } else if (abs(newTime.secsTo(KeSysStart))>2 && abs(newTime.secsTo(KeSysStart))<5) {
        res = 1;
        emit sysTimeUpdated();
    } else if (abs(newTime.secsTo(KeSysStart))>5) {
        res = 2; //Was events
        emit sysTimeRewrited();
    } else res = 3;
    KeSysStart=newTime;
    sysTimeTimer.stop();
    if (res==0 && !inputs.isEmpty())
        emitIfReady();
}

void KeTcpObject::AutoRefreshStart()
{
    keSend("$KE,DAT,ON");
}
void KeTcpObject::AutoRefreshStop()
{
    keSend("$KE,DAT,OFF");
}
void KeTcpObject::WriteOutputs(quint16 outputs)
{
    keSend(QByteArray("$KE,WRA,").append((const char*)&outputs, sizeof(outputs)));
}
void KeTcpObject::WriteRelay(int relay, bool sate)
{
    if ((relay>0) && (relay<5))
        keSend(QString("$KE,REL,%1,%2").arg(relay).arg(sate));
}

void KeTcpObject::WriteSingleRelay(int relay, bool sate)
{
    if ((relay>0) && (relay<5))
        for (int i=1;i<5;i++)
            keSend(QString("$KE,REL,%1,%2").arg(i).arg((i==relay) && sate));
}

void KeTcpObject::refreshSysTime()
{
    if(!sysTimeTimer.isActive()) {
        sysTimeTimer.start();
        keSend("$KE,IMPL,1");
    }
}

void KeTcpObject::watchAdcStart(int channel, int valveIn, int valveOut, int resetIn, int delay, int limit, int interval)
{
    watchAdcTimer.disconnect();
    QObject::connect(this,&KeTcpObject::inputChanged, [this,valveIn,delay](int input, bool res){
        if(((input==valveIn) && res) && (!watchAdcTimer.isActive()))
            watchAdcTimer.start(delay*1000);
        });
    QObject::connect(&watchAdcTimer, &QTimer::timeout, [this,channel,interval,limit,resetIn,valveOut](){
        keSend(QString().number(channel).prepend("$KE,ADC,")); 
        AdcZero[channel] = Adc[channel];
        if (AdcZero[channel]!=0)
            outputs[valveOut] = (abs(AdcZero[channel] - Adc[channel])*100/AdcZero[channel]>limit) && !(inputs.testBit(resetIn));
        qDebug() << getDeviceName() << "valveOut =" << outputs[valveOut] << ";  Adc val =" <<Adc[channel];
        watchAdcTimer.setInterval(interval*1000);
        });
    watchAdcTimer.start(delay*1000);
}

void KeTcpObject::watchAdcStop()
{
    watchAdcTimer.stop();
}

void KeTcpObject::watchCodesStart()
{
    icsTimer.setSingleShot(true);
    icsTimer.setInterval(KE_WAITFOR_SABILE);
    QObject::connect(&icsTimer, &QTimer::timeout, this,
                     [=](){
                           emit inputCodeChanged(inputCode);
                           });
    QObject::connect(this,  &KeTcpObject::inputsChanged,
                     [=](const QBitArray &inputs){
        icsTimer.start();
        int inCnt=(inputs.count()<inputCodeSize?inputs.count():inputCodeSize);
        QByteArray byteArray(inCnt,0);
        for (int i=0;i<inCnt;++i)
            byteArray[i/8]=(byteArray.at(i/8)|((inputs.at(i)?1:0)<<(i%8)));
        inputCode=*(quint32 *)byteArray.constData();
    });
}

void KeTcpObject::setAuxProperties(const QString &auxProperties)
{
    QVariantMap objProperties = QJsonDocument::fromJson(auxProperties.toUtf8()).object().toVariantMap();
    const QMetaObject *metaObj = this->metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i)
        if (objProperties.contains(metaObj->property(i).name()))
            metaObj->property(i).write(this,objProperties.value(metaObj->property(i).name()));
}
