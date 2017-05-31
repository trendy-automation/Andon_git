#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <QTimer>
#include <QMetaObject>
#include <QMetaProperty>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <QApplication>
#include <QThread>
#include <QHostAddress>
#include <QtConcurrent>
//#include "qjsonrpctcpserver.h"
//#include "watchdog.h"

template<class T>
QVariantMap cfGetProperties(T * obj,const QStringList &requested)
{
    QVariantMap objProperties;
    const QMetaObject *metaObj = T::metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i)
        if (requested.contains(metaObj->property(i).name()) || requested.isEmpty())
            objProperties.insert(QString(metaObj->property(i).name()), metaObj->property(i).read(obj));
    return objProperties;
}

template<class T>
void cfSetProperties(T * obj,const QVariantMap &objProperties)
{
    const QMetaObject *metaObj = obj->metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i)
        if (objProperties.contains(metaObj->property(i).name()))
            metaObj->property(i).write(obj,objProperties.value(metaObj->property(i).name()));
}

template<class T>
void cfListenPort(T * obj, quint16 port, int interval, int delay/*,std::function<void()>functor=[](){}*/) {
    QTimer *listenTimer = new QTimer;
    QObject::connect(listenTimer,&QTimer::timeout,/*obj,*/[obj,port,listenTimer,interval/*,functor*/](){
        //qDebug()<<"currentThread()"<<QThread::currentThread();
        //QFuture<bool> Listening = QtConcurrent::run(/*(T*)*/obj, T::listen,QHostAddress::AnyIPv4, port);
//        QFuture<bool> Listening = QtConcurrent::run(/*obj,*/[obj,port]()->bool{return obj->listen(QHostAddress::AnyIPv4, port);});
//        if (Listening.result()) {
            if (obj->listen(QHostAddress::AnyIPv4, port)) {
//                functor();
                qDebug()<<QString("%1: %2 port opened").arg(obj->objectName()).arg(port);
                listenTimer->stop();
                listenTimer->deleteLater();
            } else {
                qDebug()<<QString("%1: Failed to open port %2").arg(obj->objectName()).arg(port);
                listenTimer->start(interval);
            }
    }/*, Qt::QueuedConnection*/);
    //QTimer::singleShot(0,obj,[listenTimer,delay](){listenTimer->start(delay);});
    listenTimer->start(delay);
    //listenTimer->moveToThread(obj->thread());
}

template<class T>
T* cfGetObject(const QString &objectName)
{
    T *obj =qApp->findChild<T*>(objectName);
    if(obj)
        return obj;
    //qDebug()<<QString("Object %1 not found in App!").arg(objectName);
    QVariant pointer = qApp->property(objectName.toLatin1());
    if(pointer.isValid())
        obj = (T *)pointer.value<void *>();
    if(obj)
        return obj;
    qDebug()<<QString("Object %1 not found in App properties!").arg(objectName);
    return 0;
}




//static void appStartWatchdog(int port)
//{
//    qDebug()<<"Start Watchdog";
//    Watchdog* watchdog = getObject<Watchdog>("watchdog");
////    watchdog = qApp->findChild<Watchdog*>("watchdog");
////    if(!watchdog){
////        qDebug()<<"Object watchdog not found in App!";
////        return;
////    }
//    if(!watchdog)
//        return;
////    WatchdogRpcService * watchdogRpcService = new WatchdogRpcService(qApp);
////    watchdogRpcService->setObjectName("serverRpcService");
//    QJsonRpcTcpServer * watchdogRpcServer = new QJsonRpcTcpServer(qApp);
////    watchdogRpcServer->setObjectName("watchdogRpcServer");
////    watchdogRpcServer->addService(watchdogRpcService);
//    watchdogRpcServer->addService(watchdog);
//    listenPort<QJsonRpcTcpServer>(watchdogRpcServer,port,3000,700,&watchdog->startProcess);
//}

#endif // COMMON_FUNCTIONS_H
