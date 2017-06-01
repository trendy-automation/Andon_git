#include <QTcpSocket>
#include "client_rpcutility.h"
#include <QHostAddress>
#include <QEventLoop>
#include <QApplication>
#include <QTimer>
#include <QNetworkProxyFactory>


ClientRpcUtility::ClientRpcUtility(QObject *parent)
    : QObject(parent),
      m_client(0)
{
    //    QThread *thread=new QThread();
    //    this->moveToThread(thread);
    //    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    //    QObject::connect(thread, &QThread::finished, this, &ClientRpcUtility::deleteLater);
    //    QObject::connect(this, &ClientRpcUtility::Query2Json, this, &ClientRpcUtility::sqlQuery,Qt::UniqueConnection);
    //    thread->start();
}

void ClientRpcUtility::setEngine(QJSEngine* SharedEngine)
{
    engine=SharedEngine;
}

QJsonRpcServiceReply *ClientRpcUtility::ServerExecute(const QString &RemoteMethodName, const  QVariantList &inParameterList,
                                                      std::function<void(QVariant)> functor)
{
//    qDebug()<<RemoteMethodName<<InParameterList;
    QTcpSocket *socket = new QTcpSocket();//(this);
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    socket->connectToHost(serveraddress/*.toString()*/, JSONRPC_SERVER_PORT/*,
                          QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol*/);
    if (!socket->waitForConnected()) {
        //qDebug() << "could not connect to server: " << socket->errorString();
        emit error(QString("Error ""%1"" trying execute %2").arg(socket->errorString()).arg(RemoteMethodName));
        functor(QVariant());
        return 0;
    }
    m_client = new QJsonRpcSocket(socket);//(socket,this);
    QVariant arg[10];
    for (int i=0;i<9;++i)
        arg[i]=i<inParameterList.count()?inParameterList.at(i):QVariant();
    QJsonRpcServiceReply *reply = m_client->invokeRemoteMethod(QString(JSONRPC_SERVER_SERVICENAME).append(".").append(RemoteMethodName),
                                                               arg[0],arg[1],arg[2],arg[3],arg[4],arg[5],arg[6],arg[7],arg[8],arg[9]);
    if(functor)
        QObject::connect(reply, &QJsonRpcServiceReply::finished, [reply,functor] () {
            QVariant result = reply->response().result().toVariant();
            if (result.isValid())
                functor(result);
            else
                qDebug() << "invalid response received!!!"
                         << reply->response().errorMessage();
        });
    return reply;
}

void ClientRpcUtility::ServerExecute(const QString &RemoteMethodName, QVariantList InParameterList,
                                     QJSValue scriptFunctor)
{
    std::function<void(QVariant response)> functor=0;
    qDebug()<<"scriptFunctor.isCallable()"<<scriptFunctor.isCallable();
    if(engine && scriptFunctor.isCallable()){
        //        qDebug() << "scriptFunctor callable";
        functor = [this,scriptFunctor](QVariant response){
            QJSValue result = QJSValue(scriptFunctor).call( //scriptFunctor.call(
                                                            QJSValueList() << engine->toScriptValue(response));
            if (result.isError()){
                qDebug() << "Uncaught exception at line"
                         << result.property("lineNumber").toInt()
                         << ":" << result.toString()
                         << "in script:" << scriptFunctor.property("name").toString();
            }
            else {
                if (result.isVariant()){
                    qDebug()<<result.toVariant();
                    return result.toVariant();
                }
            }
        };
    }
    else {
        qDebug()<<"Default scriptFunctor";
        functor=[] (QVariant response){ qDebug()<<"scriptFunctor is not a function."<<response; };
    }
    ServerExecute(RemoteMethodName, InParameterList, functor);
}

//void ClientRpcUtility::Query2Json(const QString &queryText, std::function<void(QVariant)> functor)
//{
//    ServerExecute("SQLQuery2Json", QVariantList()<<queryText, functor);
//}

void ClientRpcUtility::Query2Json(const QString &queryText, std::function<void(QVariant)> functor)
{
    ServerExecute("SQLQuery2Json", QVariantList()<<queryText, [functor](QVariant resp){
        if(functor)
            functor(resp);
    });
}

void ClientRpcUtility::Query2Json(const QString &queryText, QJSValue scriptFunctor)
{
    ServerExecute("SQLQuery2Json", QVariantList()<<queryText, scriptFunctor);
}

void ClientRpcUtility::setserverip(QHostAddress newserveraddress)  {
    serveraddress = newserveraddress;
}

QVariant ClientRpcUtility::evaluate(const QString &scriptText)
{
    if(engine){
        QJSValue result = engine->evaluate(scriptText);
        if (result.isError())
            qDebug() << "Uncaught exception at line"
                     << result.property("lineNumber").toInt()
                     << ":" << result.toString()
                     << "in script:" << scriptText;
        if (result.isVariant())
            return result.toVariant();
    }
    return QVariant();
}

QVariant ClientRpcUtility::query(const QString &queryText)
{
    QEventLoop replyWaitLoop;
    QJsonRpcServiceReply * reply = ServerExecute("SQLQuery2Json", QVariantList()<<queryText);
    if (reply){
        QObject::connect(reply, &QJsonRpcServiceReply::finished, &replyWaitLoop, &QEventLoop::quit);
        //TODO signal jsonrpc timeout
        QTimer::singleShot(JSONRPC_REPLY_TIMEOUT, &replyWaitLoop, &QEventLoop::quit);
        replyWaitLoop.exec();
        if (!reply->response().result().toVariant().isValid())
            qDebug() << "invalid response received!!!"
                     << reply->response().errorMessage();
        else
            return reply->response().result().toVariant();
    }
    return QVariant();
}




