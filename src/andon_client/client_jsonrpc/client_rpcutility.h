#ifndef CLIENT_RPCUTILITY_H
#define CLIENT_RPCUTILITY_H

#include <QObject>
#include <QHostAddress>

#include "json/qjsonvalue.h"
#include "qjsonrpcsocket.h"
#include "qjsonrpcservice.h"
#include "qjsonrpcservicereply.h"
#include <QJsonDocument>
//#include <QtScript>
#include <QJSEngine>
#include <QJSValue>
#include <functional>

//Q_DECLARE_METATYPE (std::function<void(QVariant)>)
//Q_DECLARE_SMART_POINTER_METATYPE(std::function<QVariant(QVariant)>)

class QJsonRpcSocket;
class ClientRpcUtility : public QObject
{
    Q_OBJECT

//    template<typename T> struct FunctorType
//    {
//        typedef std::function<T(QVariant)> Type ;
//    };
//    FunctorType<int>::Type Funcp = func<int> ;

public:
    ClientRpcUtility(QObject *parent = 0);
    void setserverip(QHostAddress newserveraddress);



public slots:
    void setEngine(QJSEngine *SharedEngine);
    QJsonRpcServiceReply *ServerExecute(const QString &RemoteMethodName, const QVariantList &inParameterList,
                        std::function<void(QVariant)> functor=0);
    void ServerExecute(const QString &RemoteMethodName, QVariantList InParameterList,
                        QJSValue scriptFunctor);
    /*template <> */void Query2Json(const QString &queryText, std::function<void(QVariant)> functor=0);
    //void Query2Json(const QString &queryText, const std::function<void(QVariant)>& functor=0);
    void Query2Json(const QString &queryText, QJSValue scriptFunctor);
    QVariant query(const QString &queryText);
    QVariant evaluate(const QString &scriptText);

signals:
    void error(QString errorString);
    void server_proc_reply(QString replyString);
    void connectionRefusedError();
//    void Query2Json(const QString &queryText, std::function<void(QVariant)> functor=0);
private:
    QJsonRpcSocket *m_client;
    QHostAddress serveraddress;
    QJSEngine* engine;
//protected:
//    virtual void run(void);
};

#endif
