#include "server_rpcservice.h"
#include "qjsonrpcsocket.h"
//#include "qjsonrpcservice.h"
#include "common_functions.h"
//#include <QtConcurrent>

QString AtIndexOf(QStringList ResList,QStringList IndexList, QString indexVal)
{
    if (IndexList.contains(indexVal))
        return ResList.at(IndexList.indexOf(indexVal));
    else return QString();
}

ServerRpcService::ServerRpcService(QObject *parent)
    : QJsonRpcService(parent)
{
    andonDb=cfGetObject<DBWrapper>("andonDb");
}

ServerRpcService::~ServerRpcService()
{
}

QString ServerRpcService::curClientIp()
{
    //qDebug()<<"curClientIp()";
    QString address;
    QVariant adr(this->property("address"));
    if(adr.isValid())
        address==adr.toString();
    if (address.isEmpty()){
        //qDebug()<<"this address is empty";
        QJsonRpcAbstractSocket *socket = this->currentRequest().socket();
        if(socket)
            address=socket->property("address").toString();
    }
    if (address.isEmpty())
        qDebug()<<"address is empty";
    return address;
}



void ServerRpcService::StopSms(const QString &EVENT_ID)
{         
    QString ClientIP = curClientIp();
    QStringList SmsKeys;
    if (ThreadsMap.contains(ClientIP)) {
        if (EVENT_ID=="ALL")
            SmsKeys = ThreadsMap[ClientIP].keys();
        else
            SmsKeys.append(EVENT_ID);
        for(int i=0;i<ThreadsMap[ClientIP].count();i++) {
            if (ThreadsMap[ClientIP].contains(EVENT_ID)) {
//                qDebug()<<"ThreadsMap[ClientIP][EVENT_ID]->quit();";
                ThreadsMap[ClientIP][EVENT_ID]->quit();
                ThreadsMap[ClientIP][EVENT_ID]->exit();
                ThreadsMap[ClientIP][EVENT_ID]->wait(2);
                //QThread: Destroyed while thread is still running
                qDebug()<<"ThreadsMap[ClientIP][EVENT_ID]->deleteLater();";
                ThreadsMap[ClientIP][EVENT_ID]->deleteLater();
                qDebug()<<"ThreadsMap[ClientIP].remove(EVENT_ID);";
                ThreadsMap[ClientIP].remove(EVENT_ID);
            }
        }
    }
}

void ServerRpcService::PauseSms(const QString &EVENT_ID)
{
    QString ClientIP = curClientIp();
    if (ThreadsMap.contains(ClientIP))
        if (ThreadsMap[ClientIP].contains(EVENT_ID))
            ThreadsMap[ClientIP][EVENT_ID]->exit();
}

void ServerRpcService::ResumeSms(const QString &EVENT_ID)
{
    QString ClientIP = curClientIp();
    if (ThreadsMap.contains(ClientIP))
        if (ThreadsMap[ClientIP].contains(EVENT_ID))
            ThreadsMap[ClientIP][EVENT_ID]->start();
}

void ServerRpcService::CancelSms(const QString &EVENT_ID)
{
    QString ClientIP = curClientIp();    
    QStringList SmsKeys;
    if (ThreadsMap.contains(ClientIP)) {
        if (EVENT_ID=="ALL")
            SmsKeys = ThreadsMap[ClientIP].keys();
        else
            SmsKeys.append(EVENT_ID);
        for(int i=0;i<ThreadsMap[ClientIP].count();i++) {
            if (ThreadsMap[ClientIP].contains(SmsKeys.at(i)))
                ThreadsMap[ClientIP][SmsKeys.at(i)]->setObjectName("Cancelled");
            //            ThreadsMap[ClientIP][SmsKeys.at(i)]->exit();
            //            ThreadsMap[ClientIP][SmsKeys.at(i)]->wait(1);
            //            ThreadsMap[ClientIP][SmsKeys.at(i)]->deleteLater();
            //            ThreadsMap[ClientIP].remove(SmsKeys.at(i));
        }
    }
}

//QString ServerRpcService::StartSms(const QString &sms_params)
QString ServerRpcService::StartSms(const QString &sms_params)
{
    QString ClientIP = curClientIp();
    //qDebug()<<QString(sms_params).replace(":CLIENT_IP",ClientIP);
    QString result;
    //qDebug()<<"SMS_params"<<sms_params;
    QVariantMap ParamsMap = QJsonDocument::fromJson(sms_params.toUtf8()).object().toVariantMap();

    bool newSms=true;
    QString EVENT_ID;
    if(ParamsMap.contains("EVENT_ID"))
        EVENT_ID=ParamsMap["EVENT_ID"].toString();


    //qDebug()<<"curClientIP"<<ClientIP;
    if (ThreadsMap.contains(ClientIP))
        if (ThreadsMap[ClientIP].contains(EVENT_ID)) {
            //qDebug()<<"ClientIP"<< ClientIP <<"EVENT_ID"<<EVENT_ID<<"allready exist";
            newSms=false;
        }
    if (newSms) {
        QString queryText=QString("SELECT * FROM SERVER_SELECT_SMS('%1')").arg(sms_params);//).replace(":CLIENT_IP",ClientIP);
        //if (queryText.contains(":CLIENT_IP"))
        //    queryText.replace(":CLIENT_IP",ClientIP);

        qDebug() << "queryText" <<queryText;
        //QString sqlqueryres = andonDb->query2json(queryText);
        QString sqlqueryres = SQLQuery2Json(queryText);
        //qDebug()<<"SMS sqlqueryres" << sqlqueryres;

        QJsonDocument jdocSms;
        if (!sqlqueryres.isEmpty())
            jdocSms= QJsonDocument::fromJson(sqlqueryres.toUtf8());
        QJsonObject SmsObject = jdocSms.object();
        QStringList SmsNameList = SmsObject["FieldList"].toString().split(",");
        SmsObject.remove("FieldList");
        if (SmsObject.keys().count()>0) {
            //qDebug()<<"SMS 2";
            for(int x=0; x < SmsObject.keys().count(); x++) {
                //qDebug()<<"SMS 3";
                QStringList SmsValList = SmsObject[SmsObject.keys().at(x)].toString().split("|");
                bool ok = false;
                AtIndexOf(SmsValList,SmsNameList,"PHONE_NUMBER").toDouble(&ok);
                if (ok) {
                    //qDebug()<<"SMS 4";

                    if (!ThreadsMap.contains(ClientIP)) {
                        QMap<QString,QThread*> map;
                        ThreadsMap.insert(ClientIP,map);
                    }
                    if (!ThreadsMap[ClientIP].contains(EVENT_ID)) {
                        QThread *NewThread =new QThread;
                        NewThread->setProperty("ClientIP",ClientIP);
                        NewThread->setProperty("EVENT_ID",EVENT_ID);
                        ThreadsMap[ClientIP].insert(EVENT_ID,NewThread);

                        //QObject::connect(NewThread, &QThread::destroyed, [=](){
                        //    qDebug()<<"destroyed ThreadsMap[ClientIP].remove(EVENT_ID);";
                        //    if (ThreadsMap.contains(NewThread->property("ClientIP").toString()))
                        //        if (ThreadsMap[NewThread->property("ClientIP").toString()].contains(NewThread->property("EVENT_ID").toString()))
                        //            ThreadsMap[NewThread->property("ClientIP").toString()].remove(NewThread->property("EVENT_ID").toString());
                        //});

                        //TODO: not need?
                        //ThreadsMap[ClientIP][EVENT_ID]->setProperty("TIMER_COUNT",0);
                    }

                    ThreadsMap[ClientIP][EVENT_ID]->setProperty("TIMER_COUNT",
                                                              ThreadsMap[ClientIP][EVENT_ID]->property("TIMER_COUNT").toInt()+1);

                    //TODO: QThreadPool instead of Map
                    QTimer *newtimer = new QTimer(0);
                    newtimer->moveToThread(ThreadsMap[ClientIP][EVENT_ID]);
                    //qDebug()<< "newtimer->moveToThread" << ThreadsMap[ClientIP][EVENT_ID];
                    //TODO: Put sms parameters to JSON
                    newtimer->setInterval(AtIndexOf(SmsValList,SmsNameList,"SMS_TIMEOUT").toInt()*60000);
                    //TODO: Put sms delay to sender class
                    //if (newtimer->interval()==0)
                    //    newtimer->setInterval(x*5000);
                    newtimer->setSingleShot(true);
                    newtimer->setProperty("PHONE_NUMBER",AtIndexOf(SmsValList,SmsNameList,"PHONE_NUMBER"));
                    newtimer->setProperty("LANG_NAME",AtIndexOf(SmsValList,SmsNameList,"LANG_NAME"));
                    newtimer->setProperty("LOG_SMS_ID",AtIndexOf(SmsValList,SmsNameList,"LOG_SMS_ID").toInt());
                    newtimer->setProperty("SMS_TEXT",AtIndexOf(SmsValList,SmsNameList,"SMS_TEXT"));
                    newtimer->setProperty("CANCEL_TEXT",AtIndexOf(SmsValList,SmsNameList,"CANCEL_TEXT"));
                    result.append(AtIndexOf(SmsValList,SmsNameList,"SMS_TIMEOUT")).append(" ")
                            .append(AtIndexOf(SmsValList,SmsNameList,"WORK_POSITION"));
                    //qDebug()<<"SMS 5";
                    QObject::connect(newtimer,&QTimer::timeout,[=](){
                        emit SendSMS(newtimer->property("PHONE_NUMBER").toString(),
                                           newtimer->property("SMS_TEXT").toString(),
                                           newtimer->property("LANG_NAME").toString(),
                                           newtimer->property("LOG_SMS_ID").toInt(),0);


                        //newtimer->setInterval(999999999);
                        int timercount = newtimer->thread()->property("TIMER_COUNT").toInt();
                        if (timercount<=1) {
                            qDebug()<<"timercount<=1";
                            newtimer->thread()->exit();
                            //newtimer->thread()->wait(5);
                            //qDebug()<<"thread()->wait(5)";
//                            qDebug()<<"newtimer->thread()->quit()";
                            newtimer->thread()->quit();

                            //newtimer->thread()->terminate();
                            //qDebug()<<"thread()->terminate()";
                          //newtimer->thread()->deleteLater();
                          //qDebug()<<"thread()->deleteLater";
                        } else
                            newtimer->thread()->setProperty("TIMER_COUNT",timercount-1);
                    });

                    QObject::connect(ThreadsMap[ClientIP][EVENT_ID], &QThread::started, newtimer,
                                     static_cast<void (QTimer::*)()>(&QTimer::start));
                    QObject::connect(ThreadsMap[ClientIP][EVENT_ID], &QThread::finished, newtimer, &QTimer::stop);
                    QObject::connect(ThreadsMap[ClientIP][EVENT_ID], &QThread::objectNameChanged,[=](){
                        qDebug()<<"cancel sms";
                        if (!newtimer->isActive()) {
                            qDebug()<<"cancel sms sended";
                            emit SendSMS(newtimer->property("PHONE_NUMBER").toString(),
                                               newtimer->property("CANCEL_TEXT").toString(),
                                               newtimer->property("LANG_NAME").toString(),
                                               newtimer->property("LOG_SMS_ID").toInt(),0);
                            qDebug()<<"newtimer->deleteLater";
                            newtimer->deleteLater();
                        } else {
                            int timercount = newtimer->thread()->property("TIMER_COUNT").toInt();
                            qDebug()<<"timercount"<<timercount;
                            if (timercount<=1) {
//                                qDebug()<<"thread()->quit()";
                                newtimer->thread()->quit();
                                //newtimer->thread()->wait(10);
                                //qDebug()<<"thread()->wait(10)";

                                //newtimer->thread()->terminate();
                                //qDebug()<<"thread()->terminate";

                                //delete newtimer;
                                qDebug()<<"newtimer->deleteLater";
                                newtimer->deleteLater();


                                //delete NewThread;
                                //qDebug()<<"thread()->deleteLater";
                            } else {
                                newtimer->thread()->setProperty("TIMER_COUNT",timercount-1);
                                qDebug()<<"delete newtimer";
                                delete newtimer;
                                //newtimer->deleteLater();
                                //qDebug()<<"newtimer->deleteLater";
                            }

                            qDebug()<<"end cancelling sms";
                        }
                    });

                    QObject::connect(ThreadsMap[ClientIP][EVENT_ID], &QThread::destroyed, newtimer, &QTimer::deleteLater);



                } else qDebug()<<"PHONE_NUMBER"<<AtIndexOf(SmsValList,SmsNameList,"PHONE_NUMBER")<< "Not integer" ;
            }
            //qDebug()<<"SMS 6";
            //TODO: No need condition?
            if (ThreadsMap.contains(ClientIP))
                if (ThreadsMap[ClientIP].contains(EVENT_ID))
                    if (!ThreadsMap[ClientIP][EVENT_ID]->isRunning())
                        ThreadsMap[ClientIP][EVENT_ID]->start();
        } //else qDebug()<<"No result for query:"<<queryText;
        return sqlqueryres;
    } else qDebug()<<"newSMS="<<newSms;
    //qDebug()<<"SMS 7";
    return QString();
}

/*std::function<QString (QString)>*/ QString ServerRpcService::SQLQuery2Json(QString queryText)
{
//    return [=](QString queryText)->QString{
    // qDebug() << queryText;
    if (queryText.isEmpty()) {
        qDebug() << "SQL query empty in SQLQuery2Json";
        return QString();
    } else{
        if (queryText.contains(":CLIENT_IP"))
            queryText.replace(":CLIENT_IP",curClientIp().append("\'").prepend("\'"));
        //qDebug() << queryText;
        //return andonDb->query2json(queryText);
//        QString res;
//        QMetaObject::invokeMethod(andonDb, "query2json", Qt::BlockingQueuedConnection,
//                                  Q_RETURN_ARG(QString, res),
//                                  Q_ARG(QString, queryText));
//        return res;
        QFuture<QString> future = QtConcurrent::run(andonDb,&DBWrapper::query2json, queryText);
        return future.result();
    }
//    }(queryText);
}

void ServerRpcService::executeProc(const QString & queryText)
{
    qDebug() << queryText;
    if (queryText.isEmpty()) {
        qDebug() << "SQL query empty in executeProc";
        return;
    } else
        //QFuture<bool> future = QtConcurrent::run(andonDb,&DBWrapper::executeProc, QString(queryText).replace(":CLIENT_IP",curClientIp().append("\'").prepend("\'")));
        andonDb->executeProc(QString(queryText).replace(":CLIENT_IP",curClientIp().append("\'").prepend("\'")));
}

