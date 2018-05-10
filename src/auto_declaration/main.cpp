#include "plc_station.h"
#include "message_handler.h"
#include "qttelnet.h"
#include "single_apprun.h"
//#include "watchdog.h"
#include <QApplication>
#include <QProcess>
#include <QNetworkProxy>
#include <QSettings>
#include <QTimer>

QString shieldPath(const QString &anyPath)
{
    QString resPath(anyPath);
            QRegExp rx("/((\\w+\\s+)+\\w+)/");
            if(rx.indexIn(anyPath)!=-1){
                QStringList list = rx.capturedTexts();
                list.removeDuplicates();
                for(QString &s:list){
                    if(!s.endsWith(" ") && !s.endsWith("/")){
                        resPath.replace(s,QString("\"%1\"").arg(s));
                    }
                }
            }
            return resPath;
}

void StartApp()
{
    qDebug()<<"StartApp start";

    QString FileName("settings.ini");
    QSettings settings(FileName, QSettings::IniFormat);


    //########### Step 1.0 ############
    QtTelnet *telnetClient = new QtTelnet;

    //########### Step 2 TELNET_SAP ############
    qDebug() << "new TELNET_SAP";
    //Mitsu_FC1	22	10.224.29.60	SAPcoRPC	Faurecia01*
    telnetClient->setObjectName("Mitsu_FC1");
    telnetClient->setProperty("ID_TCPDEVICE", 22);
    telnetClient->setHost("10.224.29.60");
    telnetClient->setTelnetUser("SAPcoRPC");
    telnetClient->setTelnetPass("Faurecia01*");
    telnetClient->start();
    QTimer* keepConnectionTimer = new QTimer;
    QObject::connect(keepConnectionTimer, &QTimer::timeout,[=](){
        //nonexist kanban
        //13:07:46.316 StartApp 48 kanban error 7 "errNoKanban infTaskFinished Kanban 1598000000 not maintained in ZJK00 table"
        telnetClient->kanbanDeclare(1,"1598000000", "RUTYABC018", "initial", 11);
    });
    QObject::connect(telnetClient, &QtTelnet::kanbanFinished,
                     [=] (int logKanbanId,const QByteArray &kanbanNumber, int error, int idDevice, const QString &message){
        keepConnectionTimer->start(3000000);
        if(error==0)
            qDebug() << "KANBAN DECLARED" << kanbanNumber << "->" << message;
        else
            qDebug() << "kanban error" << error <<message;
    });
    QObject::connect(telnetClient, &QtTelnet::serverBrokeTheConnection,[] (){
        qDebug() << "serverBrokeTheConnection lambda";
        QString appPath(shieldPath(qApp->applicationDirPath()));
        QString cmdCommand("cmd.exe /C start %1 %2.exe %3");
        cmdCommand = cmdCommand.arg(appPath).arg(qApp->applicationName()).arg(APP_OPTION_FORCE);
        QProcess *restartApp = new QProcess;
        restartApp->startDetached(cmdCommand);
    });

    //########### Step 3 PLC_PARTNER connect ############
    //192.168.0.11
    //{"LocalAddress":"192.168.0.10", "LocTsap":"1002", "RemTsap":"2002",
    //"users":["RUTYABC018", "initial","RUTYABC019", "initial"]}
//    QByteArray LocalAddress("192.168.0.10");
//    QByteArray RemoteAddress("192.168.0.11");
    bool ok;
//    int LocTsap=QString("1002").toInt(&ok,16);
//    int RemTsap=QString("2002").toInt(&ok,16);

    QByteArray LocalAddress  = settings.value("LocalAddress","192.168.0.10").toByteArray();
    QByteArray RemoteAddress = settings.value("RemoteAddress","192.168.0.11").toByteArray();
    int LocTsap = settings.value("LocTsap",QString("1002").toInt(&ok,16)).toInt();
    int RemTsap = settings.value("RemTsap",QString("2002").toInt(&ok,16)).toInt();

    settings.setValue("LocalAddress", LocalAddress);
    settings.setValue("LocTsap", LocTsap);
    settings.setValue("RemoteAddress", RemoteAddress);
    settings.setValue("RemTsap", RemTsap);


    Plc_station * plcPartner = new Plc_station;
//    QObject::connect(plcPartner, &Plc_station::reqDeclKanban,
//                     [telnetClient](const QByteArray &kanbanNumber, const QByteArray &user, const QByteArray &pass, int idDevice){
//            QTimer::singleShot(0,telnetClient,[telnetClient,kanbanNumber,user,pass,idDevice](){
//                telnetClient->kanbanDeclare(1, kanbanNumber,user,pass,idDevice);
//            });
//    });
    QObject::connect(plcPartner, &Plc_station::reqDeclKanban, telnetClient, &QtTelnet::kanbanDeclare,Qt::QueuedConnection);
    QObject::connect(telnetClient, &QtTelnet::kanbanFinished, plcPartner, &Plc_station::resDeclKanban);
    QObject::connect(telnetClient, &QtTelnet::serverBrokeTheConnection, plcPartner, &Plc_station::kanbanDeclError);
    plcPartner->setObjectName("DP_4B45X");
    plcPartner->setIdDevice(13);
    plcPartner->setUsers(QVariantList()<<"RUTYABC018"<<"initial"<<"RUTYABC019"<<"initial");
    plcPartner->StartTo(LocalAddress,RemoteAddress,(word)LocTsap,(word)RemTsap);
    QStringList plcStatusList;
    plcStatusList<<"stopped"<<"running and active connecting"<<"running and waiting for a connection"
                 <<"running and connected : linked"<<"sending data"<<"receiving data"
                 <<"error starting passive server";
    qDebug()<<"plcPartner status:"<<plcPartner->getStatus()<<plcStatusList.at(plcPartner->getStatus());
    qDebug()<<"StartApp finished";




}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QStringList args = a.arguments();
//    Watchdog *watchdog = new Watchdog(&a);
//    watchdog->setObjectName("watchdog");
//    if(args.contains(APP_OPTION_WATHCDOG)){
//        if(!watchdog->listen(JSONRPC_CLIENT_WATCHDOG_PORT,QString(JSONRPC_WATCHDOG_SERVICENAME).append(".isAlive")))
//            qDebug() << "Watchdog application cannot run!";
//        return a.exec();
//    }
    QByteArray textCodec="cp1251";
    if (!qApp->applicationDirPath().toLower().contains("build"))
        textCodec="cp866";
    /*****************************************
     * Start MessageHandler
     *****************************************/
    MessageHandler msgHandler(textCodec);
    /*****************************************
     * Start SingleAppRun
     *****************************************/
    qDebug()<<"Start SingleAppRun";
    SingleAppRun singleApp(args.contains(APP_OPTION_FORCE),&a);
    if(singleApp.isToQuit()){
        a.quit();
        return 0;
    }
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    StartApp();
//    /*****************************************
//     * Start Watchdog
//     *****************************************/
//    qDebug()<<"Start Watchdog";
//    watchdog->startRpcServer(JSONRPC_CLIENT_WATCHDOG_PORT);
    return a.exec();
}
