#include <QApplication>

#include "qjsonrpctcpserver.h"
#include "server_rpcservice.h"
#include "server_rpcutility.h"
#include "sms_service.h"
#include "bcsender.h"
#include "iostreamtheard.h"
#include "dbwrapper.h"
#include "sendemail.h"
#include "message_handler.h"
#include "single_apprun.h"

//________WebUI____________
#include "serverwebthread.h"
#include "qwebchannel.h"
#include <QtWebSockets/QWebSocketServer>
#include "websocketclientwrapper.h"
#include "websockettransport.h"

//_______QtTelnet class_______________
//#include "qttelnet.h"

#include <functional>
#include <QJSEngine>
#include "main.h"

#include "watchdog.h"

#include <QMessageBox>
#include <QVariant>
//#include <QtConcurrent>
//#include <QThreadPool>

#include "excel_report.h"

//#include <QNetworkProxyFactory>

int main(int argc, char *argv[])
{
    //qRegisterMetaType<QJsonRpcTcpServer*>("QJsonRpcTcpServer");
    QApplication a(argc, argv);
    /*****************************************
     * Start Watchdog
     *****************************************/
    QStringList args = a.arguments();
    Watchdog *watchdog = new Watchdog/*(&a)*/;
    watchdog->setObjectName("watchdog");
    if(args.contains(APP_OPTION_WATHCDOG)){
        if(!watchdog->listen(JSONRPC_SERVER_WATCHDOG_PORT,QString(JSONRPC_WATCHDOG_SERVICENAME).append(".isAlive")))
            //appClientDisconnected();
            qDebug() << "Watchdog application cannot run!";
        return a.exec();
    }
//    else
//        QTimer::singleShot(10000,[](){
//            qDebug() << "Test crash application";
//            QObject*null;
//            null->setObjectName("crash");
//        });
    /*****************************************
     * Start MessageHandler
     *****************************************/
    MessageHandler msgHandler;//("cp866")

    QNetworkProxyFactory::setUseSystemConfiguration(false);
    /*****************************************
     * Start Single Application
     *****************************************/
    qDebug()<<"Start singleApp";
    SingleAppRun singleApp(args.contains(APP_OPTION_FORCE),&a);
    if(singleApp.isToQuit()){
        a.quit();
        return 0;
    }

    /*****************************************
     * Start DataBase
     *****************************************/
    DBWrapper *andonDb = new DBWrapper;
    andonDb->setObjectName("andonDb");
    a.setProperty("andonDb", qVariantFromValue((void *) andonDb));
//    QFuture<bool> futureDb = QtConcurrent::run(andonDb,&DBWrapper::ConnectDB,qApp->applicationDirPath(),QString(DB_DATABASE_FILE));
//    if(!futureDb.result()){
//    QThread andonDbThread;
//    andonDb->moveToThread(&andonDbThread);
//    andonDbThread.start();
//    QTimer::singleShot(0,andonDb,[andonDb](){
    if(!andonDb->ConnectDB(qApp->applicationDirPath(),DB_DATABASE_FILE)){
        //qDebug()<<"ConnectDB failed";
        QMessageBox::information(new QWidget,"Andon server failed","Can not connect to DB");
//        andonDbThread.quit();
        qApp->quit();
        return 0;
    }
//    });
    QObject::connect(andonDb,&DBWrapper::dbError,watchdog,&Watchdog::rebootPC);
    /*****************************************
     * Start QtTelnet
     *****************************************/
    //    qDebug()<<"Start telnetClient";
    //    QtTelnet * telnetClient = new QtTelnet;
    //    telnetClient->setObjectName("telnetClient");
    //    telnetClient->start();
    /*****************************************
     * Start serverRpcService
     *****************************************/
    qDebug()<<"Start serverRpcService";
    ServerRpcService * serverRpcService = new ServerRpcService;
    serverRpcService->setObjectName("serverRpcService");
    a.setProperty("serverRpcService", qVariantFromValue((void *) serverRpcService));
    QJsonRpcTcpServer * rpcServer = new QJsonRpcTcpServer;
//    qDebug()<<"rpcServer->thread()"<<rpcServer->thread();
    rpcServer->setObjectName("rpcServer");
    a.setProperty("rpcServer", qVariantFromValue((void *) rpcServer));
    rpcServer->addService(serverRpcService);
    rpcServer->setMaxPendingConnections(50);
//    QThread rpcServerThread;
//    serverRpcService->moveToThread(&rpcServerThread);
//    rpcServer->moveToThread(&rpcServerThread);
//    rpcServerThread.start();
    QObject::connect(rpcServer, &QJsonRpcTcpServer::clientConnected, rpcServer, appClientConnected);
    QObject::connect(rpcServer, &QJsonRpcTcpServer::clientDisconnected, rpcServer, appClientDisconnected);
//    QtConcurrent::run([rpcServer](){cfListenPort<QJsonRpcTcpServer>(rpcServer,JSONRPC_SERVER_PORT,3000,700);});
//    QtConcurrent::run(/*std::bind(*/reinterpret_cast<void(QJsonRpcTcpServer*,int,int,int)>(cfListenPort)
//                                ,rpcServer,JSONRPC_SERVER_PORT,3000,700/*)*/);
//    QtConcurrent::run((std::function<void(QJsonRpcTcpServer*,int,int,int)>)cfListenPort
//                                ,rpcServer,JSONRPC_SERVER_PORT,3000,700);
    //QTimer::singleShot(0,rpcServer,[rpcServer](){
        cfListenPort<QJsonRpcTcpServer>(rpcServer,JSONRPC_SERVER_PORT,3000,700);
        QTimer *listenTimer = new QTimer;
        QObject::connect(listenTimer,&QTimer::timeout,[rpcServer](){
            if (!rpcServer->isListening())
                if(!rpcServer->property("opening").toBool())
                    cfListenPort<QJsonRpcTcpServer>(rpcServer,JSONRPC_SERVER_PORT,3000,0);
        });
        listenTimer->start(3600000); //each 1 hour

    //});
//    QtConcurrent::run(QThreadPool::globalInstance(),
//                      static_cast<void(QJsonRpcTcpServer*, short unsigned int, int, int,std::function<void>)>(cfListenPort),
//                      //(void(QJsonRpcTcpServer*, short unsigned int, int, int,std::function<void>))(cfListenPort),
//                      rpcServer,(quint16)JSONRPC_SERVER_PORT,3000,700,[](){});
//    /*****************************************
//     * Start clientRpcService
//     *****************************************/
//    qDebug()<<"Start clientRpcService";
//    ServerRpcUtility * clientRpcService = new ServerRpcUtility;
//    clientRpcService->setObjectName("clientRpcService");
    /*****************************************
     * Start Unicast UDP Sender
     *****************************************/
    qDebug()<<"Start Unicast UDP Sender";
    //TODO: server startup delay to DB
    BCSender * bcSender = new BCSender(UDP_INTERVAL,UDP_PORT,&a);
    bcSender->setObjectName("bcSender");
//    QObject::connect(andonDb, &DBWrapper::DBConnected, andonDb, [bcSender,andonDb](){
        andonDb->executeQuery("SELECT IP_ADDRESS FROM TBL_STATIONS WHERE ENABLED='1'","json",
                              [bcSender](const QString &jsonClients){
                                  //QtConcurrent::run(bcSender,&BCSender::addClients,jsonClients);
                               bcSender->addClients(jsonClients);
        });
//    });
    /*****************************************
     * Start ioStreamThread
     *****************************************/
    qDebug()<<"Start ioStreamThread";
    ioStreamThread * iotheard = new ioStreamThread;
    iotheard->setObjectName("iotheard");
    QObject::connect(iotheard, &ioStreamThread::inputReceived, appParseInput);
    QObject::connect(iotheard, &ioStreamThread::started, bcSender, &BCSender::run);
    QTimer::singleShot(2000,iotheard,&ioStreamThread::startThread);
    /*****************************************
     * Start WebUI
     *****************************************/
    qDebug()<<"Start WebUI";
    QWebSocketServer webSocketServer(QStringLiteral("QWebChannel Server"), QWebSocketServer::NonSecureMode,&a);
    webSocketServer.setObjectName("webSocketServer");
    cfListenPort<QWebSocketServer>(&webSocketServer,WEBCHANNEL_PORT,3000,500);

    WebSocketClientWrapper clientWrapper(&webSocketServer);
    WebuiThread *WThread = new WebuiThread;
    WThread->setObjectName("WebuiThread");
    WThread->start();
    //QTimer::singleShot(0,WThread,[WThread](){
    cfListenPort<WebuiThread>(WThread,(quint16)WUI_PORT,3000,1000);
    //});
    //    qDebug()<<"Start setup the channel";
    QWebChannel channel(&a);
    QObject::connect(&clientWrapper, &WebSocketClientWrapper::clientConnected,
                     &channel, &QWebChannel::connectTo);
    channel.registerObject(QStringLiteral("serverWeb"), WThread);
    channel.registerObject(QStringLiteral("db"), andonDb);
//    QObject::connect(WThread,static_cast<void (WebuiThread::*)(const QString &sql_query,
//                                                               std::function<void(QSqlQuery /***/query)> functor)>(&WebuiThread::getSqlQuery),
//                     andonDb, static_cast<void (DBWrapper::*)(const QString &sql_query,
//                                                              std::function<void(QSqlQuery *query)> functor)>(&DBWrapper::executeQuery));

//    QObject::connect(WThread,static_cast<void (WebuiThread::*)(const QString &sql_query,const QString &query_method,
//                                                               std::function<void(const QString &jsontext)> functor)>(&WebuiThread::getSqlQuery),
//                     andonDb, static_cast<void (DBWrapper::*)(const QString &sql_query,const QString &query_method,
//                                                              std::function<void(const QString &jsontext)> functor)>(&DBWrapper::executeQuery));
    QObject::connect(WThread,&WebuiThread::getSqlQuery,andonDb,&DBWrapper::executeQuery);
    /*****************************************
    * Start reportTimer
    *****************************************/
    //    appExecuteReport(QString("SELECT * FROM REPORT_BREAKDOWNS('%1', '%2')")
    //                     .arg(QDate(QDate::currentDate().year(),QDate::currentDate().month(),1).toString("dd.MM.yyyy"))
    //                     .arg(QDate(QDate::currentDate().year(),(QDate::currentDate().month()+1)%12,1).toString("dd.MM.yyyy")),
    //                     QDate::currentDate().toString("Простои"),
    //        QString("P:\\!Common Documents\\Andon_reports\\Простои за прошлый месяц"),
    //        "brakedowns");

    //    appExecuteReport("SELECT * FROM PRODUCTION_DECLARATION_HISTORY", "AutoDecl",
    //        QString("P:\\!Common Documents\\AutomaticDeclarating\\AutoDecl_export"),"AutoDecl_aria");

    qDebug()<<"Start reportTimer";

    ExcelReport *excelReport=new ExcelReport;
//    QThread excelReportThread;
//    excelReport->moveToThread(&excelReportThread);
//    excelReportThread.start();
    channel.registerObject(QStringLiteral("excelReport"), excelReport);
    a.setProperty("excelReport", qVariantFromValue((void *) excelReport));
    const int msecsPerDay = 24 * 60 * 60 * 1000;
    QTimer * reportTimer = new QTimer(QAbstractEventDispatcher::instance());
    reportTimer->setTimerType(Qt::VeryCoarseTimer);
    QDateTime cdt = QDateTime::currentDateTime();
    reportTimer->start(qMax((qint64)10000, cdt.msecsTo(QDateTime(cdt.date(),QTime(23,50)))));
    qDebug()<<"reportTimer start"<<reportTimer->interval()/3600000.0<<"hours";
    QObject::connect(reportTimer,&QTimer::timeout, excelReport, [excelReport,reportTimer,msecsPerDay,andonDb](){
        //qDebug()<<"reportTimer timeout"<<"dayOfWeek"<<QDate::currentDate().dayOfWeek();
//        excelReport->queryText2File("SELECT * FROM REPORT_MONTH_DECLARATION", "AutoDecl",
//                         QString("P:\\!Common Documents\\AutomaticDeclarating\\AutoDecl_%1.xlsx")
//                         .arg(QDate::currentDate().toString("MMMM_yyyy")),"AutoDecl_aria");
        excelReport->queryText2File("SELECT * FROM MNT_MOLD_REPORT", "Andon_cycle_counter",
                         "P:\\Maintenance\\Обслуживание пресс-форм\\Andon_cycle_counter.xlsx");
        /*if(QDate::currentDate().daysInMonth()==QDate::currentDate().day())
            excelReport->queryText2File(QString("SELECT * FROM REPORT_BREAKDOWNS('%1', '%2')")
                             .arg(QDate(QDate::currentDate().year(),QDate::currentDate().month(),1).toString("dd.MM.yyyy"))
                             .arg(QDate(QDate::currentDate().year(),(QDate::currentDate().month()+1)%12,1).toString("dd.MM.yyyy")),
                             QDate::currentDate().toString("Простои"),
                             QString("P:\\!Common Documents\\Andon_reports\\Простои за прошлый месяц.xlsx"),
                             "brakedowns");*///
//        if(QDate::currentDate().dayOfWeek()<6)
//            QTimer::singleShot(0,andonDb,[andonDb,excelReport](){
//            andonDb->executeQuery("SELECT LIST(EMAIL) FROM TBL_STAFF WHERE EMAIL_REPORTING=1","json",
//                                  [excelReport](const QString &jsonUsers){
//                QJsonArray arr=QJsonDocument::fromJson(jsonUsers.toUtf8()).array();
//                if(!arr.isEmpty()){
//                    QJsonObject obj=arr.at(0).toObject();
//                    if(!obj.isEmpty()){
//                        QStringList rcpnts=obj.value(obj.keys().at(0)).toString().split(',');
//                        if(!rcpnts.isEmpty())
//                            QTimer::singleShot(1,excelReport,[excelReport,rcpnts](){
//                            excelReport->queryText2Email("SELECT * FROM REPORT_BREAKDOWNS",
//                                                     QString("Простои производства %1").arg(QDate::currentDate().toString("ddd d MMMM")),rcpnts,"",
//                                                     QString("Отчёт по простоям %1").arg(QDate::currentDate().toString("ddd d MMMM")),
//                                                     QString("REPORT_BREAKDOWNS_%1.xlsx").arg(QDate::currentDate().toString("dd_MM_yyyy")));
//                            });
//                    }
//                }
//            });
//            });
        QDateTime cdt = QDateTime::currentDateTime();
        reportTimer->start(cdt.msecsTo(QDateTime(cdt.addDays(1).date(),QTime(23,50))));
        //reportTimer->start(msecsPerDay-qMax(QTime(23,50).elapsed(),
        //                                    QTime(23,50).elapsed())+1000);
        qDebug()<<"reportTimer start"<<reportTimer->interval()/3600000.0<<"hours";
    });

    /*
    QObject::connect(schedulerTimer,&QTimer::timeout,[&schedulerList,schedulerTimer](){
        for(auto s : schedulerList) {
            if (!s.eventTimer){
                s.eventTimer=new QTimer;
                QObject::connect(s.eventTimer,&QTimer::timeout,s.functor);

                QDateTime nextEvent;
                QTime curTime=QTime::currentTime();
                QDate curDate=QDate::currentDate();
                QDate checkDate=curDate;
                for(int i=0;i<31;++i) {
                    for(auto s : schedulerList)
                        for(auto f : s.eventFloatTimes) {
                            if (f.dayOfMonth==checkDate.day() && (checkDate!=curDate || f.eventTime>curTime)) {
                                nextEvent=curDate;
                                nextEvent.setTime(f.eventTime);
                                s.eventTimer->start(QDateTime::msecsTo(nextEvent));
                                break;
                            }
                            if (f.eventWeekdays.contains(checkDate.dayOfWeek()) && (checkDate!=curDate || f.eventTime>curTime)) {
                                nextEvent;
                                nextEvent.setTime(f.eventTime);
                                s.eventTimer->start(QDateTime::msecsTo(nextEvent));
                                break;
                            }
                        }

                    checkDate.addDays(1)
                }
            }
        }

    });
*/

    /*****************************************
     * Start SMS Server
     *****************************************/
    qDebug()<<"Start SMS Server";
    Sms_service * sms_sender = new Sms_service;
    QObject::connect(serverRpcService,&ServerRpcService::SendSMS, sms_sender,
                     &Sms_service::sendSMSFECT,Qt::QueuedConnection);
    QObject::connect(sms_sender,&Sms_service::SmsStatusUpdate,andonDb,[andonDb]
                     (int SmsLogId, int SmsId, int Status){
        andonDb->executeProc(QString("EXECUTE PROCEDURE SERVER_UPDATE_SMSSTATUS(%1, %2, %3)")
                             .arg(SmsLogId).arg(SmsId).arg(Status));
    });


//    QFuture<QString> future = QtConcurrent::run(andonDb,&DBWrapper::query2json, QString("SELECT AUX_PROPERTIES_LIST "
//                                                                                "FROM TBL_TCPDEVICES "
//                                                                                "WHERE DEVICE_TYPE='SMS Server' AND DEVICE_TYPE='SMS Server'"));
    QString sqlqueryres = /*future.result();*/
                            andonDb->query2json("SELECT AUX_PROPERTIES_LIST "
                                              "FROM TBL_TCPDEVICES "
                                              "WHERE DEVICE_TYPE='SMS Server' AND DEVICE_TYPE='SMS Server'");
    QJsonDocument jdocURL;
    if (!sqlqueryres.isEmpty()){
        jdocURL= QJsonDocument::fromJson(sqlqueryres.toUtf8());
        QJsonArray URLArray = jdocURL.array();
        QJsonObject URLObject = URLArray.at(0).toObject();
        if (URLObject.contains("AUX_PROPERTIES_LIST")){
            jdocURL = QJsonDocument::fromJson(URLObject["AUX_PROPERTIES_LIST"].toString().toUtf8());
            URLObject = jdocURL.object();
            sms_sender->setURL(URLObject["URL"].toString());
        }
    } else
        sms_sender->setURL("http://10.208.98.101:81/sendmsg?user=SMS1&passwd=smssms1&cat=1&enc=%1&to=%2&text=%3");
//    QThread smsSenderThread;
//    sms_sender->moveToThread(&smsSenderThread);
//    smsSenderThread.start();
    /*****************************************
     * Start SendEmail
     *****************************************/
    qDebug()<<"Start SmtpClient";
    //TODO From DB

    SendEmail * emailClient = new SendEmail;
    emailClient->setObjectName("emailClient");
    QObject::connect(emailClient,&SendEmail::errorMessage,[](const QString & message){
        qDebug()<<"Email errorMessage"<<message;
    });

    /*****************************************
     * Start Script Engine
     *****************************************/
    qDebug()<<"Start Script Engine";

    QJSEngine *engine = new QJSEngine;
    engine->globalObject().setProperty("msgHandler",engine->newQObject(&msgHandler));
    //engine->globalObject().setProperty("telnetClient",engine->newQObject(telnetClient));
    engine->globalObject().setProperty("serverRpcService",engine->newQObject(serverRpcService));
    engine->globalObject().setProperty("sms_sender",engine->newQObject(sms_sender));
    engine->globalObject().setProperty("andonDb",engine->newQObject(andonDb));
    engine->globalObject().setProperty("excelReport",engine->newQObject(excelReport));
    engine->globalObject().setProperty("emailClient",engine->newQObject(emailClient));
    engine->globalObject().setProperty("WThread",engine->newQObject(WThread));
    engine->globalObject().setProperty("rpcServer",engine->newQObject(rpcServer));

    WThread->setEngine(engine);


    /*****************************************
     * Start ServerStartScript Evaluate
     *****************************************/
    qDebug()<<"Start ServerStartScript Evaluate";

    QStringList ScriptsList;
//    QFuture<QString> future2 = QtConcurrent::run(andonDb,&DBWrapper::query2json, QString("SELECT SCRIPT_TEXT "
//                                                                                "FROM TBL_SCRIPTS WHERE SCRIPT_NAME='ScriptServerStart'"));
    QString sqlqueryScripts /*= future2.result()*/;

    QJsonDocument jdocScripts(QJsonDocument::fromJson(/*andonDb->query2json(
                                                          QString("SELECT SCRIPT_TEXT "
                                                                  "FROM TBL_SCRIPTS WHERE SCRIPT_NAME='ScriptServerStart'"))*/sqlqueryScripts.toUtf8()));
    QJsonArray tableArray = jdocScripts.array();
    QJsonObject recordObject;
    if (!tableArray.isEmpty())
        recordObject=tableArray.at(0).toObject();

    QList<QJsonObject> scriptObjectList;
    for (int i=0;i<tableArray.count();++i)
        scriptObjectList.append(tableArray.at(i).toObject());
    for (int i=0;i<scriptObjectList.count();++i){
        QString ScriptText = scriptObjectList.at(i)["SCRIPT_TEXT"].toString().replace("`","'");
        //        QScriptSyntaxCheckResult result = engine->checkSyntax(ScriptText);
        //        if (result.state()==QScriptSyntaxCheckResult::Valid) {
        //            ScriptsList.append(ScriptText);
        //        }
        //        else {
        //            qDebug()<<"Syntax error in script "<<"("<<result.errorLineNumber()<<
        //                      ","<<result.errorColumnNumber()<<"):";
        //            qDebug()<<ScriptText<<"\nMessage:"<<result.errorMessage();
        //        }
    }
    for (int i = 0; i<ScriptsList.count(); ++i) {
        //qDebug()<<"server_start"<<ScriptsList.at(i);
        engine->evaluate(ScriptsList.at(i));
    }

    /*****************************************
     * Start Watchdog
     *****************************************/
    qDebug()<<"Start Watchdog";
    watchdog->startRpcServer(JSONRPC_SERVER_WATCHDOG_PORT);
    QObject::connect(watchdog,&Watchdog::processRestart,[/*serverRpcService*/sms_sender](){
        /*QJsonObject joClient;
        joClient.insert("STATION_IP",QHostAddress(QHostAddress::LocalHost).toString());
        joClient.insert("EVENT_ID",QTime::currentTime().toString("HH:mm:ss.zzz"));
        joClient.insert("STATUS", "DISCONNECTED");
        joClient.insert("USER_COMMENT", "Server restart!");
        QJsonDocument jdClient(joClient);*/
        sms_sender->sendSMSFECT("89657009502", "Server restart!","RU",789);
    });
    if(QTime::currentTime().hour()==0){
        /*QJsonObject joClient;
        joClient.insert("STATION_IP",QHostAddress(QHostAddress::LocalHost).toString());
        joClient.insert("EVENT_ID",QTime::currentTime().toString("HH:mm:ss.zzz"));
        joClient.insert("STATUS", "DISCONNECTED");
        joClient.insert("USER_COMMENT", "Server is running!");
        QJsonDocument jdClient(joClient);*/
        sms_sender->sendSMSFECT("89657009502", "Server is running!","RU",987);
        qDebug()<<"Server running SMS";
    }
//    QtConcurrent::run(excelReport,&ExcelReport::queryText2File,QString("SELECT * FROM REPORT_MONTH_DECLARATION"), QString("AutoDecl"),
//                      QString("P:\\!Common Documents\\AutomaticDeclarating\\AutoDecl_export.xlsx"),QString("AutoDecl_aria"));
//    QtConcurrent::run(excelReport,&ExcelReport::queryText2File,QString("SELECT * FROM MNT_MOLD_REPORT"), QString("Andon_cycle_counter"),
//                      QString("P:\\Maintenance\\Обслуживание пресс-форм\\Andon_cycle_counter.xlsx"),QString("AutoDecl_aria"));
//    QTimer::singleShot(15000,excelReport,[excelReport](){
//        excelReport->queryText2File("SELECT * FROM REPORT_MONTH_DECLARATION", "AutoDecl",
//                         QString("P:\\!Common Documents\\AutomaticDeclarating\\AutoDecl_export.xlsx")
//                         //.arg(QDate::currentDate().toString("MMMM_yyyy"))
//                         ,"AutoDecl_aria");
//        excelReport->queryText2File("SELECT * FROM MNT_MOLD_REPORT", "Andon_cycle_counter",
//                         "P:\\Maintenance\\Обслуживание пресс-форм\\Andon_cycle_counter.xlsx");
//    });
    qDebug()<<"main finish";
    return a.exec();
}
