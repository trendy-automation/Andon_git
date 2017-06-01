
#include "interface_manager.h"
#include "udpreceiver.h"
#include "Plc_station.h"
#include "message_handler.h"
#include "qttelnet.h"
#include "single_apprun.h"
#include "qftp.h"
#include "main_callbacks.h"
#include "main.h"
//#include "serlock_manager.h"
#include "watchdog.h"

//#include "opcua_client.h"

#include <QApplication>
//#include <cmath>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

//#include "client_rpcservice.h"
//#include "qjsonrpctcpserver.h"
#include <QJSEngine>
//#include "websocketclientwrapper.h"
//#include "websockettransport.h"
//#include "clientwebinterface.h"
//#include "qwebchannel.h"
//#include <QtWebSockets/QWebSocketServer>
#include <QtQml>

//#include "math.h"
//using namespace std;
//#include <functional>

//#include <QProcess>
using namespace ML;

struct QueryTemplate{
    QStringList fields;
    QVariantList args;
};
//TODO class appLoader;
//QVariant Query2Json(QString query, );
//On eventloop
//void QueryFunctor(QStringList)
//{
//    serverRpc->Query2Json(QString(
//                         "SELECT %1 FROM %2 (%3)")
//                             .arg(value.toInt()).arg(deviceId), [](QVariant resp){
//        QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
// //                        qDebug()<<"PRODUCTION_PART_PRODUSED"<<array.toVariantList();
//        for (auto object = array.begin(); object != array.end(); object++) {
//            QJsonObject jsonObj=object->toObject();
//            if (jsonObj.contains("DEVICE_NAME") && jsonObj.contains("PART_NAME"))
//                qDebug()<<jsonObj["DEVICE_NAME"].toString()<<jsonObj["PART_NAME"].toString();
//        }
//    });
//}
//OR signal - slot queryId OR qftp style OR StataMachine
//QMap<QString,QueryTemplate> queryMap;
//{{},{}};

void ServerFound(QHostAddress ServerAddress)
{
    qDebug()<<"ServerFound start";
    InterfaceManager* IM = new InterfaceManager;
    IM->InitVKeyboard();
    ClientRpcUtility *serverRpc = new ClientRpcUtility(qApp);
    MLserverRpc=serverRpc;
    //    serverRpc->start();
    QObject::connect(serverRpc,&ClientRpcUtility::error,[=](QString errorString){
        qDebug()<<errorString;
        IM->setEnabled(false);
        UdpReceiver* udpreceiver = qApp->findChild<UdpReceiver*>();
        if(udpreceiver){
            udpreceiver->disconnect();
            QObject::connect(udpreceiver, &UdpReceiver::serverfound, [IM,ServerAddress] (QHostAddress newServerAddress){
                if(ServerAddress==newServerAddress)
                    IM->setEnabled(true);
            });
            udpreceiver->start();
        }

        /*        if (QApplication::applicationDirPath().toLower().contains("build"))
            return;
        //QProcess process; //Anyway Reboot!
        //process.startDetached("shutdown.exe -r -f -t 0");
        IM->setWindowFlags(Qt::WindowStaysOnBottomHint);
        QMessageBox msgBox;
        msgBox.setText("Fail connection: Cannot connect to server!");
        msgBox.setInformativeText("Do you want to reboot now?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
        int ret = msgBox.exec();
        if (ret==QMessageBox::Ok) {
            QProcess process;
            process.startDetached("shutdown.exe -r -f -t 0");
        } else if (ret==QMessageBox::Cancel) {
            IM->setEnabled(true);
            IM->setWindowFlags(Qt::WindowStaysOnTopHint);
            IM->show();
        }
*/
    });
    serverRpc->setObjectName("serverRpc");
    serverRpc->setserverip(ServerAddress);
    QJSEngine *engine = new QJSEngine;
    engine->installExtensions(QJSEngine::ConsoleExtension);

    //    QJSValue result = myEngine.evaluate('');
    //      if (result.isError())
    //          qDebug() << "Uncaught exception at line"
    //                   << result.property("lineNumber").toInt()
    //                   << ":" << result.toString();



    MessageHandler* msgHandler = qApp->findChild<MessageHandler*>();
    if(msgHandler)
        engine->globalObject().setProperty("msgHandler",engine->newQObject(msgHandler));
    engine->globalObject().setProperty("IM",engine->newQObject(IM));
    engine->globalObject().setProperty("serverRpc",engine->newQObject(serverRpc));
    //    QJSValue ScriptFunctor = engine->newFunction(construct_Functor);
    //    QJSValue construct_QTimer = engine->evaluate("function QTimer() {}");
    //    construct_QTimer.setPrototype(engine->newQObject(new QTimer));
    //    //QJSValue constructorTimer = engine->newFunction(construct_QTimer);
    //    //QJSValue valueTimer = engine->newQMetaObject(&QPushButton::staticMetaObject, constructorTimer);
    //TODO QJSEngine *engine->newQMetaObject
    //QJSValue valueTimer = engine->newQMetaObject(&QPushButton::staticMetaObject, construct_QTimer);
    //engine->globalObject().setProperty("QTimer", valueTimer);
    //    QJSValue constructorQEngine = engine->newFunction(construct_QEngine);
    //    QJSValue valueEngine = engine->newQMetaObject(&QPushButton::staticMetaObject, constructorQEngine);
    //    engine->globalObject().setProperty("QEngine", valueEngine);
    //engine->globalObject().setProperty("engine",engine->newQObject(engine));

    IM->setServerRpc(serverRpc);
    IM->setEngine(engine);
    serverRpc->setEngine(engine);
    //    qDebug() << "serverRpc->ServerExecute";
    //    serverRpc->ServerExecute("SQLQuery2Json",
    //           QVariantList()<<"SELECT * FROM PRODUCTION_PARTS_HISTORY",
    //           engine->evaluate("(function foo(resp) { console.log('SQLQuery2Json result',resp);})"));
    //    serverRpc->ServerExecute("SQLQuery2Json",
    //           QVariantList()<<"SELECT * FROM PRODUCTION_PARTS_HISTORY",
    //           engine->evaluate("(function foo() { print('foo'); })"));


    if (QApplication::applicationDirPath().toLower().contains("build")) {
        IM->setGeometry(0,0,800,600);
        //        QJSEngineDebugger * ScriptDebuger = new QJSEngineDebugger;
        //        ScriptDebuger->action(QJSEngineDebugger::InterruptAction);
        //        //ScriptDebuger->action(QJSEngineDebugger::RunToCursorAction);
        //        ScriptDebuger->attachTo(engine);
        //        //ScriptDebuger->standardWindow()->show();
    } else {
        QApplication::setOverrideCursor(Qt::BlankCursor);
        IM->setWindowState(Qt::WindowFullScreen);
        IM->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }

    //########### Step 1.0 ############
    QtTelnet *telnetClient = new QtTelnet;


    //########### Step 1.1 ############
    serverRpc->Query2Json("SELECT RPL, WL "
                          " FROM CLIENT_SELECT_INTERFACE(:CLIENT_IP)",
                          [IM](QVariant resp){
        IM->loadInterface(resp);
    });


    //########### Step 1.2 TCP DEVICES ############
//    loadKeObjects(serverRpc,qApp);
    serverRpc->Query2Json("SELECT ID_TCPDEVICE, TCPDEVICE_IP, PORT, LOGIN, PASS, "
                          "DEVICE_NAME, DEVICE_TYPE, AUX_PROPERTIES_LIST, CLASS_NAME"
                          " FROM CLIENT_SELECT_TCPDEVICES(:CLIENT_IP)",// mcbLoadTcpDevices);
                                [=](QVariant resp){mcbLoadTcpDevices(resp);});

    serverRpc->Query2Json("SELECT ID_TCPDEVICE, TCPDEVICE_IP, PORT, LOGIN, PASS, "
                          "DEVICE_NAME, DEVICE_TYPE, AUX_PROPERTIES_LIST "
                          " FROM CLIENT_SELECT_TCPDEVICES(:CLIENT_IP)"
                          " WHERE DEVICE_TYPE = 'FTP'",
                          [=](QVariant resp){
        qDebug()<<"lambda TCPDEVICES start";
        QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
        for (auto row = array.begin(); row != array.end(); row++) {
            QJsonObject jsonRow=row->toObject();
            if (jsonRow.contains("DEVICE_TYPE") && jsonRow.contains("TCPDEVICE_IP")
                    && jsonRow.contains("LOGIN") && jsonRow.contains("PASS")) {
                if (jsonRow["DEVICE_TYPE"].toString()=="FTP") {
                    QFtp *ftp = new QFtp(jsonRow["TCPDEVICE_IP"].toString(),jsonRow["PORT"].toInt(),
                            jsonRow["LOGIN"].toString(),jsonRow["PASS"].toString());
                    QBuffer *buffer=new QBuffer;
                    QTimer *fileTimer = new QTimer;
                    fileTimer->setTimerType(Qt::VeryCoarseTimer);
                    QObject::connect(ftp, &QFtp::commandFinished,[ftp,buffer,serverRpc](int command,bool res){
                        if(command==buffer->property("command").toInt()){
                            int taskId=buffer->property("task").toInt();
                            buffer->buffer().clear();
                            buffer->close();
                            if(res){
                                taskId=-taskId;
                                qDebug() << "Ftp: file not wirted";
                            }
                            serverRpc->Query2Json(QString("SELECT PART_REFERENCE, "
                                                          "PART_COUNT FROM PRODUCTION_DECLARATING(%1)").arg(taskId),
                                                  [](QVariant resp){
//                                qDebug() << "PRODUCTION_DECLARATING finish" << resp.toString().size();
//                                QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
//                                if(!array.isEmpty()) {
//                                    QJsonObject jsonObj0=array.at(0).toObject();
//                                    if(jsonObj0.contains("PART_REFERENCE") && jsonObj0.contains("PART_COUNT")){
//                                        qDebug() << "Ftp: file writed";
//                                        for (auto object:array) {
//                                            QJsonObject jsonObj=object.toObject();
//                                            qDebug() << jsonObj["PART_REFERENCE"].toString()
//                                                    << jsonObj["PART_COUNT"].toInt();
//                                        }
//                                    }
//                                }
                            });
                        }
                    });
                    QObject::connect(fileTimer,&QTimer::timeout,
                                     [serverRpc,ftp,buffer,fileTimer](){
                        QDateTime cdt = QDateTime::currentDateTime();
                        //int interval = qMin(FTP_INTERVAL+120,qMax(FTP_INTERVAL-120, cdt.msecsTo(QDateTime(cdt.addDays(cdt.time().hour()==23?1:0), QTime(cdt.time().hour()+1,58)))));
                        QDateTime ndt = QDateTime::currentDateTime().addSecs(3600);
                        int interval = cdt.msecsTo(QDateTime(ndt.date(),QTime(ndt.time().hour(),58)));
                        //qDebug() << "fileTimer->start(" << interval << ");";
                        fileTimer->start(interval);
                        serverRpc->Query2Json("SELECT ID_TASK, PART_REFERENCE, PART_COUNT, MANUFACTURE_DATE "
                                              "FROM PRODUCTION_DECLARATING",
                                              [ftp,buffer](QVariant resp){
                            qDebug() << "PRODUCTION_DECLARATING start" << resp.toString().size();
                            QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
                            qDebug() << "PRODUCTION_DECLARATING lambda 1";
                            if(!array.isEmpty()) {
                                qDebug() << "PRODUCTION_DECLARATING lambda 2";
                                QJsonObject jsonObj0=array.at(0).toObject();
                                qDebug() << "PRODUCTION_DECLARATING lambda 3";
                                if(jsonObj0.contains("PART_REFERENCE") && jsonObj0.contains("ID_TASK")
                                        && jsonObj0.contains("PART_COUNT")) {
                                    int taskId=jsonObj0["ID_TASK"].toInt();
                                    qDebug() << "PRODUCTION_DECLARATING lambda 4";
                                    buffer->setProperty("task", taskId);
                                    buffer->open(QBuffer::ReadWrite);
                                    for (auto object = array.begin(); object != array.end(); object++) {
                                        QJsonObject jsonObj=object->toObject();
                                        buffer->write(jsonObj["PART_REFERENCE"].toString().toLatin1());
                                        buffer->write("\t");
                                        buffer->write(QString::number(jsonObj["PART_COUNT"].toInt()).toLatin1());
                                        //buffer->write("\t");
                                        //buffer->write(jsonObj["MANUFACTURE_DATE"].toString().toLatin1());
                                        buffer->write("\r\n");
                                    }
                                    qDebug() << "PRODUCTION_DECLARATING lambda 5";
                                    QTimer::singleShot(0,[ftp,buffer,taskId](){
                                        qDebug() << "PRODUCTION_DECLARATING lambda 6";
                                        buffer->setProperty("command",ftp->putBuf(buffer,
                                                                                  QString("Decl_%1.txt").arg(QDateTime().currentDateTime().toString("ddMMyy_hh_mm")), QFtp::Binary,taskId));
                                        qDebug() << "ftp putBuf lambda 7";
                                    });
                                }
                            }
                        });
                    });
                    QDateTime cdt = QDateTime::currentDateTime();
                    int interval = qMax((qint64)10000, cdt.msecsTo(QDateTime(cdt.date(),QTime(cdt.time().hour(),58))));

                    //int interval = qMax(5000,QTime::currentTime().msecsTo(QTime(QTime::currentTime().hour(),58)));

                    //qMin(FTP_INTERVAL, QTime::currentTime().msecsTo(QTime(QTime::currentTime().addMSecs(3600000).hour(),58)));

                    fileTimer->start(interval);
                    //qDebug() << "ftp cur interval" << interval;
                }
                //                if (jsonRow["DEVICE_TYPE"].toString()=="SHERLOCK") {
                //                    SherlockManager * sm = new SherlockManager(jsonRow["TCPDEVICE_IP"].toString(),
                //                                                               jsonRow["PORT"].toInt());
                //                }
            }
        }

/*
        QJsonDocument jdocKBX100(QJsonDocument::fromJson(resp.toString().toUtf8()));
        QJsonArray tableArray = jdocKBX100.array();
        QJsonObject recordObject;
        int kbx100Count=0;
        QJSValue kbx100=engine->newArray();
        engine->globalObject().setProperty("kbx100",kbx100);
        for (int i=0; i<tableArray.count();++i) {
            recordObject=tableArray.at(i).toObject();
            QString ipaddress = recordObject["TCPDEVICE_IP"].toString();
            QString pass = recordObject["PASS"].toString();
            //qDebug() << "new " << recordObject["DEVICE_TYPE"].toString();
            if (recordObject["DEVICE_TYPE"].toString()=="KBX100") {
                qDebug() << "new KBX100_Client";
                int port = recordObject["PORT"].toInt(2424);

                //if (kbx100Count==0) {
                //    }
                    KeTCP_Client *KBX100_Client = new KeTCP_Client;
                    KBX100_Client->setProperty("ID_TCPDEVICE", recordObject["ID_TCPDEVICE"].toInt());
                    //todo: KBX100_Client->setObjectName(recordObject["DEVICE_NAME"].toString());
                    KBX100_Client->setObjectName(QString("KBX100_").append(recordObject["DEVICE_NAME"].toString()));
                    QJSValue newKbx100=engine->newQObject(KBX100_Client);
                    engine->globalObject().setProperty(KBX100_Client->objectName(),newKbx100);
                    kbx100.setProperty(kbx100Count,newKbx100);
                    kbx100Count++;
                    KBX100_Client->setConnectionParams(ipaddress, port, pass);
                    if (!recordObject["AUX_PROPERTIES_LIST"].toString().isEmpty()) {
                        QJsonDocument jdocAuxProp = QJsonDocument::fromJson(recordObject["AUX_PROPERTIES_LIST"].toString().toUtf8());
                        QJsonObject jobjAuxProp = jdocAuxProp.object();
                        if (jobjAuxProp.contains("ductSensor") &&
                                jobjAuxProp.contains("channel") &&
                                jobjAuxProp.contains("valveIn") &&
                                jobjAuxProp.contains("valveOut") &&
                                jobjAuxProp.contains("resetIn") &&
                                jobjAuxProp.contains("delay") &&
                                jobjAuxProp.contains("limit") &&
                                jobjAuxProp.contains("interval")
                                ) {
                            KBX100_Client->watchAdcStart(jobjAuxProp["channel"].toInt(),jobjAuxProp["valveIn"].toInt(),
                                                        jobjAuxProp["valveOut"].toInt(),jobjAuxProp["resetIn"].toInt(),
                                                        jobjAuxProp["delay"].toInt(),jobjAuxProp["limit"].toInt(),
                                                        jobjAuxProp["interval"].toInt());
                        }

                        QObject::connect(KBX100_Client, &KeTCP_Client::AdcChange,
                                         [KBX100_Client,serverRpc] (int channel, float value){
                            serverRpc->Query2Json(QString("INSERT INTO TBL_LOG_ADC (TCPDEVICE_ID, ADC_ID, ADC_VALUE) "
                                                            "VALUES (%1,%2,%3)").arg(KBX100_Client->property("ID_TCPDEVICE").toInt())
                                                  .arg(channel).arg(value),
                                                  [=](QVariant resp){qDebug() << "WRITED ADC" << channel<<value;
                            });
                        });

                    } else
                        KBX100_Client->watchCodesStart();
                    KBX100_Client->startConnecting();
                //} else {
                //    qDebug() << "KBX100 wrong port parameter:" << recordObject["PORT"];
                //}
            }
            if (recordObject["DEVICE_TYPE"].toString()=="PLC_CLIENT") {
                TS7Client *S7Client = new TS7Client;
                S7Client->setProperty("ID_TCPDEVICE",recordObject["ID_TCPDEVICE"].toInt());
                qDebug() << "new TS7Client";
                S7Client->setObjectName("S7Client");
//                S7Client->setObjectName(QString().
//                                        number(QApplication::instance()->findChildren<TS7Client*>().count()).prepend("S7Client_"));
                engine->globalObject().setProperty(S7Client->objectName(),engine->newQObject(S7Client));
                //S7Client->ConnectTo(ipaddress.toLatin1(),0,2);
//                QString Dev_id = recordObject["ID_TCPDEVICE"].toString();
//                if (S7Client->setIP(ipaddress))
//                    S7Client->start();
//                else
//                    qDebug() << "TS7Client wrong ip address:" << ipaddress;
            }
            if (recordObject["DEVICE_TYPE"].toString()=="PLC_PARTNER") {

                Plc_station * plcPartner = new Plc_station;
                qDebug() << "new Plc_station";
                plcPartner->setObjectName(recordObject["DEVICE_NAME"].toString());
                plcPartner->setIdDevice(recordObject["ID_TCPDEVICE"].toInt());
                QByteArray RemoteAddress = recordObject["TCPDEVICE_IP"].toString().toLatin1();
                QByteArray LocalAddress;
                int LocTsap;
                int RemTsap;
                QJsonDocument jdocAuxProp = QJsonDocument::fromJson(recordObject["AUX_PROPERTIES_LIST"].toString().toUtf8());
                QJsonObject jobjAuxProp = jdocAuxProp.object();
                if (jobjAuxProp.contains("LocalAddress") &&
                        jobjAuxProp.contains("LocTsap") &&
                        jobjAuxProp.contains("RemTsap") &&
                        jobjAuxProp.contains("users")
                        ) {
                    LocalAddress = jobjAuxProp["LocalAddress"].toString().toLatin1();
                    bool ok;
                    LocTsap = jobjAuxProp["LocTsap"].toString().toInt(&ok,16);
                    RemTsap = jobjAuxProp["RemTsap"].toString().toInt(&ok,16);
                    plcPartner->setUsers(jobjAuxProp["users"].toArray().toVariantList());
                    plcPartner->StartTo(LocalAddress,RemoteAddress,(word)LocTsap,(word)RemTsap);
                    //qDebug()<<"QObject::connect plcPartner telnetClient";
                    QObject::connect(plcPartner, &Plc_station::reqDeclKanban,
                                     [serverRpc, telnetClient](const QByteArray &kanbanNumber, const QByteArray &user, const QByteArray &pass, int idDevice){
                        TelnetKanbanDeclare(serverRpc, telnetClient,
                                            kanbanNumber,user,pass,idDevice);
//                        telnetClient->kanbanDeclare(
//                                    ValFromQuery(QString("SELECT LOG_KANBAN_ID "
//                                             "FROM PROD_KANBAN_BEGIN('%1','%2')")
//                                             .arg(idDevice).arg(QString(kanbanNumber)),"LOG_KANBAN_ID").toInt()
//                                    ,kanbanNumber,user,pass,idDevice);
                    });
                    QObject::connect(telnetClient, &QtTelnet::kanbanFinished, plcPartner, &Plc_station::resDeclKanban);
                    engine->globalObject().setProperty(plcPartner->objectName(),engine->newQObject(plcPartner));
                    qDebug() << "plcPartner status" << plcPartner->getStatus();
                }
                else
                    qDebug() << "plcPartner aux properties are not full" << jobjAuxProp;
            }

            if (recordObject["DEVICE_TYPE"].toString()=="TELNET_SAP"){
                qDebug() << "new TELNET_SAP";
                telnetClient->setObjectName(recordObject["DEVICE_NAME"].toString());
                telnetClient->setProperty("ID_TCPDEVICE", recordObject["ID_TCPDEVICE"].toInt());
                    telnetClient->setHost(recordObject["TCPDEVICE_IP"].toString());
                    telnetClient->setTelnetUser(recordObject["LOGIN"].toString());
                    telnetClient->setTelnetPass(recordObject["PASS"].toString());
                engine->globalObject().setProperty(telnetClient->objectName(),engine->newQObject(telnetClient));
                telnetClient->start();
                QObject::connect(telnetClient, &QtTelnet::kanbanFinished,
                                 [=] (int logKanbanId,const QByteArray &kanbanNumber, int error, int idDevice, const QString &message){
                    qDebug() << "kanbanFinished logKanbanId error message" << logKanbanId << error <<message;
                        serverRpc->Query2Json(QString("SELECT RESULT "
                                                            "FROM PROD_KANBAN_FINISH(%1,%2,'%3')")
                                                            .arg(logKanbanId).arg(error).arg(message),
                                              [message](QVariant resp){qDebug() << "KANBAN DECLARED" << resp.toString();});
                });
            }

            if (recordObject["DEVICE_TYPE"].toString()=="COM_PORT"){
                QString portName = QString().number(recordObject["PORT"].toInt()).prepend("COM");
                qDebug() << "new COM_PORT" << portName;
                const QSerialPortInfo &portInfo = QSerialPortInfo(portName);
                QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
                foreach (const QSerialPortInfo &availablePortInfo, portList) {
                    if (availablePortInfo.serialNumber()==portInfo.serialNumber()) {
                        QSerialPort *serial = new QSerialPort(portInfo);
                        serial->setBaudRate(QSerialPort::Baud9600);
                        serial->setDataBits(QSerialPort::Data8);
                        serial->setParity(QSerialPort::NoParity);
                        serial->setStopBits(QSerialPort::OneStop);
                        serial->setFlowControl(QSerialPort::NoFlowControl);
                        serial->setProperty("LOGIN",recordObject["LOGIN"].toString());
                        serial->setProperty("PASS",recordObject["PASS"].toString());
                        serial->setProperty("ID_TCPDEVICE", recordObject["ID_TCPDEVICE"].toInt());
                        QTimer *reconnectSP = new QTimer;
                        QObject::connect(serial,&QSerialPort::readyRead,[serial, serverRpc, telnetClient, reconnectSP](){
                            QByteArray barcode(serial->readAll());
                            bool ok;
                            barcode.toInt(&ok);
                            if (barcode.length()==10 && ok){
                                qDebug()<<"scanned barcode"<<barcode;
                                TelnetKanbanDeclare(serverRpc, telnetClient,
                                                    barcode,serial->property("LOGIN").toByteArray(),
                                                    serial->property("PASS").toByteArray(),
                                                    serial->property("ID_TCPDEVICE").toInt());
//                                telnetClient->kanbanDeclare(
//                                            ValFromQuery(QString("SELECT LOG_KANBAN_ID "
//                                                     "FROM PROD_KANBAN_BEGIN('%1','%2')")
//                                                     .arg(serial->property("ID_TCPDEVICE").toInt()).arg(QString(barcode)),"LOG_KANBAN_ID").toInt()
//                                            ,barcode,serial->property("LOGIN").toByteArray()
//                                    ,serial->property("PASS").toByteArray(),serial->property("ID_TCPDEVICE").toInt());
                                reconnectSP->start(300000);
                            }
                            else
                                qDebug()<<"scanned barcode "<<barcode<<" is not kanban" ;
                        });
                        reconnectSP->setSingleShot(false);
//                        serial->setProperty("AVAILABLE",false);
                        QObject::connect(reconnectSP,  &QTimer::timeout, [serial,reconnectSP,portName](){
//                            const QSerialPortInfo &portInfo = QSerialPortInfo(portName);
//                            QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
                            serial->close();
                            serial->open(QIODevice::ReadOnly);
//                            qDebug()<<"serial->isOpen()"<<serial->isOpen()<<serial->readAll()<<serial->readBufferSize();
//                            bool portAvailable=false;
//                            foreach (const QSerialPortInfo &availablePortInfo, portList) {
//                                if (availablePortInfo.serialNumber()==portInfo.serialNumber()) {
//                                portAvailable=true;
//                                qDebug()<<"portAvailable"<<portAvailable<<"serial AVAILABLE"<<serial->property("AVAILABLE").toBool();
//                                    if (!serial->property("AVAILABLE").toBool()
//                                            || !serial->isOpen()) {
//                                        QString openRes("open success");
//                                        if (!serial->open(QIODevice::ReadOnly))
//                                            openRes=QString("open error ").append(serial->errorString());
//                                        qDebug()<<portName<<openRes;
//                                    }
//                                }
//                            }
//                            serial->setProperty("AVAILABLE",portAvailable);
//                            //if (serial->property("AVAILABLE").toBool())
                            if (reconnectSP->interval()!=30000)
                                reconnectSP->start(30000);
                        });
                        QObject::connect(serial,static_cast<void(QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error)
                                         ,[serial,reconnectSP](QSerialPort::SerialPortError error){
                                             if (error == QSerialPort::ResourceError
                                                     || error == QSerialPort::OpenError) {
                                                 qDebug()<<serial->portName()<<"COM port error"<<error;
//                                                 reconnectSP->start(30000);
                                             }
                        });
//                        QObject::connect(serial,QSerialPort::aboutToClose
//                                         ,[serial,reconnectSP](){
//                                                 qDebug()<<serial->portName()<<"COM port aboutToClose";
//                                                 reconnectSP->start(30000);
//                        });
//                        QObject::connect(serial,QSerialPort::dataTerminalReadyChanged(false)
//                                         ,[serial,reconnectSP](){
//                                                 qDebug()<<serial->portName()<<"COM port dataTerminalReadyChanged";
//                                                 reconnectSP->start(30000);
//                        });
//                        QObject::connect(serial,QSerialPort::flowControlChanged(QSerialPort::UnknownFlowControl)
//                                         ,[serial,reconnectSP](){
//                                                 qDebug()<<serial->portName()<<"COM port flowControlChanged";
//                                                 reconnectSP->start(30000);
//                        });
                        reconnectSP->start(1000);
                    }
                }
            }
        }
*/
        qDebug()<<"lambda TCPDEVICES fineshed";
    });


    //########### Step 1.3 ############
    serverRpc->Query2Json("SELECT WEBSOCKET_PORT FROM TBL_STATIONS "
                          " WHERE IP_ADDRESS = :CLIENT_IP;",
                          [=](QVariant resp){
        qDebug()<<"lambda WEBSOCKET start";

        QJsonDocument jdocWebsocketPort(QJsonDocument::fromJson(resp.toString().toUtf8()));
        //        qDebug()<<"WEBSOCKET"<< jdocWebsocketPort.toJson();
        QJsonArray tableArray = jdocWebsocketPort.array();
        QJsonObject recordObject=tableArray.at(0).toObject();
        //        qDebug()<<"WEBSOCKET"<< recordObject.toVariantMap();
        if(recordObject.contains("WEBSOCKET_PORT")){
            int websocketPort = recordObject["WEBSOCKET_PORT"].toInt();
            WebsocketInit(websocketPort, serverRpc);
        }
        qDebug()<<"lambda WEBSOCKET fineshed";

    });

    /*    //########### Step 1.4 OPC Client ############
    serverRpc->Query2Json("SELECT ID_TCPDEVICE,DEVICE_NAME FROM TBL_TCPDEVICES "
                          "WHERE DEVICE_TYPE='KBX100' AND ENABLED=1",
                          [=](QVariant resp){
            qDebug()<<"lambda OpcUaClient start";
            OpcUaClient *opcuaClient=new OpcUaClient(qApp);
            QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
            for (auto object = array.begin(); object != array.end(); object++) {
                QJsonObject jsonObj=object->toObject();
                if (jsonObj.contains("ID_TCPDEVICE") && jsonObj.contains("DEVICE_NAME"))
                    opcuaClient->appendObject(jsonObj["ID_TCPDEVICE"].toInt(),jsonObj["DEVICE_NAME"].toString());
            }
            QObject::connect(opcuaClient,&OpcUaClient::propertyChanged,
                             [opcuaClient,serverRpc](int deviceId, const QString &propertyName, const QVariant &value){
//                qDebug()<<"propertyChanged deviceId"<<deviceId<<propertyName<<value;
                if (propertyName=="inputCode" && (value.toInt()!=0)){
//                    qDebug()<<propertyName<<value.toInt();
                    serverRpc->Query2Json(QString(
                                         "SELECT DISTINCT DEVICE_NAME, MOLD_NAME FROM PRODUCTION_PART_PRODUSED (%1,%2)")
                                             .arg(value.toInt()).arg(deviceId), [](QVariant resp){
                        QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
//                        qDebug()<<"PRODUCTION_PART_PRODUSED"<<array.toVariantList();
                        for (auto object = array.begin(); object != array.end(); object++) {
                            QJsonObject jsonObj=object->toObject();
                            if (jsonObj.contains("DEVICE_NAME") && jsonObj.contains("MOLD_NAME"))
                                qDebug()<<jsonObj["DEVICE_NAME"].toString()<<jsonObj["MOLD_NAME"].toString();
                        }
                    });
                }
           });
           qDebug()<<"lambda OpcUaClient fineshed";
        });*///



    QObject::connect(IM, &InterfaceManager::interfaceLoaded, [=] (){
        qDebug()<<"interfaceLoaded";
        //########### Step 2.1 ############
        serverRpc->Query2Json("SELECT IDP, PA, PV "
                              " FROM CLIENT_SELECT_PROPERTIES(:CLIENT_IP)",
                              [IM](QVariant resp){IM->loadProperties(resp);});
        //########### Step 2.2 ############
        serverRpc->Query2Json("SELECT IDS, ST "
                              " FROM CLIENT_SELECT_SCRIPTS(:CLIENT_IP)",
                              [IM](QVariant resp){IM->loadScripts(resp);});
        //########### Step 2.3 ############
        serverRpc->Query2Json("SELECT IDW, OT, PL, SL "
                              " FROM CLIENT_SELECT_WIDGETS(:CLIENT_IP)",
                              [IM](QVariant resp){
            IM->loadWidgets(resp);
            if (((QStackedWidget*)IM->centralWidget())->count()>0)
                IM->show();//FullScreen();
        });


        QObject::connect(IM, &InterfaceManager::interfaceCreated, [=] (){
            qDebug()<<"interfaceCreated";
            //########### Step 3.1 ############
            serverRpc->Query2Json("SELECT EVENT_ID, OBJECT_NAME, OBJECT_TEXT, USER_COMMENT "
                                  " FROM CLIENT_ACTIVE_BUTTONS(:CLIENT_IP)",
                                  [IM](QVariant resp){

                QJsonDocument jdocActBtns(QJsonDocument::fromJson(resp.toString().toUtf8()));
                //qDebug()<<"jdocActBtns"<<jdocActBtns;
                QJsonArray tableArray  = jdocActBtns.array();
                for (int i=0; i<tableArray.count();++i) {
                    //qDebug()<<"tableArray.at(i).toObject()"<<tableArray.at(i).toObject();
                    IM->copyButton(QJsonDocument(tableArray.at(i).toObject()).toJson());
                }
                //qDebug()<<"tableArray"<<tableArray;
            });
        });

    });



    qDebug()<<"ServerFound fineshed";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QStringList args = a.arguments();
    Watchdog *watchdog = new Watchdog(&a);
    watchdog->setObjectName("watchdog");
    if(args.contains(APP_OPTION_WATHCDOG)){
        if(!watchdog->listen(JSONRPC_CLIENT_WATCHDOG_PORT,QString(JSONRPC_WATCHDOG_SERVICENAME).append(".isAlive")))
            qDebug() << "Watchdog application cannot run!";
        return a.exec();
    }
//    else
//        QTimer::singleShot(10000,[](){
//            qDebug() << "Test crash application";
//            QObject*null;
//            null->setObjectName("crash");
//        });
    QByteArray textCodec="cp1251";
    if (!qApp->applicationDirPath().toLower().contains("build"))
        textCodec="cp866";
    MessageHandler msgHandler(textCodec);
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    //TODO lymbda to procedures
    qmlRegisterType<InterfaceManager>("com.andon.interfacemanager", 1, 0, "InterfaceManager");
    qmlRegisterType<QTimer>("com.andon.timer", 1, 0, "QTimer");
    //    qmlRegisterType<std::function<void(QVariant)>>("com.andon.functor", 1, 0, "std::function<void(QVariant)>");
    //    qmlRegisterType<QJSEngine>("com.andon.QJSEngine", 1, 0, "QJSEngine");
    //    qmlRegisterType<QJSValue>("com.andon.QJSValue", 1, 0, "QJSValue");
    //    qmlRegisterSingletonType<QJSValue>("com.andon.QJSValue", 1, 0, "QJSValue",);
    //    qmlRegisterType<MessageHandler>("com.andon.messagehandler", 1, 0, "MessageHandler");

    //qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    //    Q_DECLARE_METATYPE( QAbstractSocket::SocketState )

    //    int qtype1 = qRegisterMetaType<QAbstractSocket::SocketError>("SocketError" );
    //    int qtype2 = qRegisterMetaType<QAbstractSocket::SocketState>("SocketState" );
    //    Q_DECLARE_METATYPE (std::function<void(QVariant)>);
    qRegisterMetaType<std::function<void(QVariant)>>("std::function<void(QVariant)>");
    qRegisterMetaType<SherlockManager*>("SherlockManager");
    qRegisterMetaType<KeTcpObject*>("KeTcpObject");

    SingleAppRun singleApp(args.contains(APP_OPTION_FORCE),&a);
    if(singleApp.isToQuit()){
        a.quit();
        return 0;
    }


//    ClientRpcService * clientrpcservice = new ClientRpcService;
//    QJsonRpcTcpServer * rpcServer = new QJsonRpcTcpServer;
//    rpcServer->addService(clientrpcservice);
//    rpcServer->setObjectName("rpcServer");
//    listenPort<QJsonRpcTcpServer>(rpcServer,JSONRPC_CLIENT_PORT,3000,1000);

    /*****************************************
     * Start UdpReceiver
     *****************************************/
    qDebug()<<"Start UdpReceiver";
    UdpReceiver *udpreceiver = new UdpReceiver(&a);
    QObject::connect(udpreceiver, &UdpReceiver::serverfound, [=] (QHostAddress ServerAddress){
        ServerFound(ServerAddress);
        // TODO: solve vkeyboard error "using null output device available"
    });
    udpreceiver->start();
    //    QObject::connect(udpreceiver, &UdpReceiver::destroyed,[](){qDebug()<<"UdpReceiver::destroyed";});
    //    QObject::connect(&msgHandler, &MessageHandler::destroyed,[](){qDebug()<<"MessageHandler::destroyed";});
    //    QObject::connect(&singleApp, &SingleAppRun::destroyed,[](){qDebug()<<"SingleAppRun::destroyed";});
    //QObject::connect(qApp, &QApplication::aboutToQuit,&msgHandler,&MessageHandler::deleteLater);
    //    QObject::connect(&a, &QApplication::aboutToQuit,[&msgHandler](){msgHandler.deleteLater();});
    //renewInterface
    QObject::connect(udpreceiver, &UdpReceiver::renewInterface, [=] (QHostAddress ServerAddress){
        qDebug() << "renewInterface";
        qDebug() << "delete QJSEngine";
        QApplication::instance()->findChild<QJSEngine*>()->deleteLater();
        qDebug() << "delete ClientRpcUtility";
        QApplication::instance()->findChild<ClientRpcUtility*>()->deleteLater();
        InterfaceManager* IM = QApplication::instance()->findChild<InterfaceManager*>();
        if (IM) {
            qDebug() << "delete IM";
            IM->close();
            IM->deleteLater();
        }
        //            ServerFound(ServerAddress);
    });
    /*****************************************
     * Start Watchdog
     *****************************************/
    qDebug()<<"Start Watchdog";
    watchdog->startRpcServer(JSONRPC_CLIENT_WATCHDOG_PORT);
    return a.exec();
}
