
#include "Plc_station.h"
#include "message_handler.h"
#include "qttelnet.h"
#include "single_apprun.h"
#include "watchdog.h"
#include <QApplication>

void RunPLC_connection()
{
    qDebug()<<"RunPLC_connection start";

    //########### Step 1.0 ############
    QtTelnet *telnetClient = new QtTelnet;


    //########### Step 1.1 ############


    //########### Step 1.2 TELNET_SAP ############
    qDebug() << "new TELNET_SAP";
    //Mitsu_FC1	22	10.224.29.60	SAPcoRPC	Faurecia01*
    telnetClient->setObjectName("Mitsu_FC1");
    telnetClient->setProperty("ID_TCPDEVICE", 22);
    telnetClient->setHost("10.224.29.60");
    telnetClient->setTelnetUser("SAPcoRPC");
    telnetClient->setTelnetPass("Faurecia01*");
    telnetClient->start();
    QObject::connect(telnetClient, &QtTelnet::kanbanFinished,
                     [=] (int logKanbanId,const QByteArray &kanbanNumber, int error, int idDevice, const QString &message){
        if(error==0)
            qDebug() << "KANBAN DECLARED" << kanbanNumber << "->" << message;
        else
            qDebug() << "kanban error" << error <<message;
    });

    //########### Step 1.3 PLC_PARTNER connect ############
    //192.168.0.11
    //{"LocalAddress":"192.168.0.10", "LocTsap":"1002", "RemTsap":"2002",
    //"users":["RUTYABC018", "initial","RUTYABC019", "initial"]}
    QByteArray LocalAddress("192.168.0.10");
    QByteArray RemoteAddress("192.168.0.11");
    int LocTsap=1002;
    int RemTsap=2002;
    Plc_station * plcPartner = new Plc_station;
    qDebug() << "PLC_PARTNER connect from "<<LocalAddress  << "to" << RemoteAddress;
    plcPartner->setObjectName("DP_4B45X");
    plcPartner->setIdDevice(13);
    plcPartner->setUsers(QVariantList()<<"RUTYABC018"<<"initial"<<"RUTYABC019"<<"initial");
        plcPartner->StartTo(LocalAddress,RemoteAddress,(word)LocTsap,(word)RemTsap);
        QObject::connect(plcPartner, &Plc_station::reqDeclKanban,
                         [telnetClient](const QByteArray &kanbanNumber, const QByteArray &user, const QByteArray &pass, int idDevice){
            telnetClient->kanbanDeclare(0, kanbanNumber,user,pass,idDevice);
        });
    QObject::connect(telnetClient, &QtTelnet::kanbanFinished, plcPartner, &Plc_station::resDeclKanban);
    qDebug() << "plcPartner status" << plcPartner->getStatus();

qDebug()<<"RunPLC_connection fineshed";
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
    QByteArray textCodec="cp1251";
    if (!qApp->applicationDirPath().toLower().contains("build"))
        textCodec="cp866";
    /*****************************************
     * Start MessageHandler
     *****************************************/
    qDebug()<<"Start MessageHandler";
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
    RunPLC_connection();
    /*****************************************
     * Start Watchdog
     *****************************************/
    qDebug()<<"Start Watchdog";
    watchdog->startRpcServer(JSONRPC_CLIENT_WATCHDOG_PORT);
    return a.exec();
}
