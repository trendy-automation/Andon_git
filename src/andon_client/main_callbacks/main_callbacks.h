#ifndef MAIN_CALLBACKS_H
#define MAIN_CALLBACKS_H

#include "client_rpcutility.h"
#include "ketcp_object.h"
#include "serlock_manager.h"

#include "common_functions.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

// main_lambdas name space


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

namespace ML{

    /*extern*/ ClientRpcUtility *MLserverRpc;

    //TODO наследовать KeObject  из TcpDevice
    template<class T>
    QList<T*> loadObjects(QVariant resp, QObject * parent=0){
        QList<T*> list;
        for(auto i:QJsonDocument::fromJson(resp.toString().toUtf8()).array()){
            if(!i.isObject()){
                qDebug() << QJsonValue(i).toVariant() << "is not an json object";
                continue;
            }
            T * obj=new T(parent);
            list << obj;
            QJsonObject jsonObj=i.toObject();
            for (auto it = jsonObj.constBegin(); it != jsonObj.constEnd(); it++)
                obj->setProperty(it.key().toLatin1(), it.value().toVariant());
        }
        return list;
    }

    static void mcbPartProdused(int partCode, int idDevice)
    {
        ClientRpcUtility*serverRpc=cfGetObject<ClientRpcUtility>("serverRpc");
        if(serverRpc)
            serverRpc->Query2Json(QString("SELECT DISTINCT DEVICE_NAME, "
                                          "MOLD_NAME FROM PRODUCTION_PART_PRODUSED (%1,%2)")
                                                 .arg(partCode).arg(idDevice),printResp);
    }
    void productionProdused(ClientRpcUtility *MLserverRpc, KeTcpObject* keObject, const QString &ioName, const QVariant &value){
        if ((ioName.compare("inputCode")==0) && ((value.toInt()!=0))){
            MLserverRpc->Query2Json(QString(
                                 "SELECT DISTINCT DEVICE_NAME, MOLD_NAME FROM PRODUCTION_PART_PRODUSED (%1,%2)")
                                     .arg(value.toInt()).arg(keObject->property("ID_TCPDEVICE").toInt()),printResp);
        }
    }

    void loadKeObjects(ClientRpcUtility *MLserverRpc,QObject * parent=0){
        //ID_TCPDEVICE, TCPDEVICE_IP, PORT, DEVICE_TYPE, DEVICE_NAME,
        //AUX_PROPERTIES_LIST, PASS, LOGIN, STATION_ID, ENABLED
        MLserverRpc->Query2Json("SELECT * FROM TBL_TCPDEVICES "
                              "WHERE DEVICE_TYPE='KBX100' AND ENABLED=1",[=](QVariant resp){
            QList<KeTcpObject*> list;
            list << loadObjects<KeTcpObject>(resp,parent);
            for (auto keObject:list){
                QObject::connect(keObject,&KeTcpObject::IOEvent,[=](const QString &ioName, const QVariant &value){
                    productionProdused(MLserverRpc, keObject, ioName, value);});
                keObject->startConnecting();
            }
        });
    }
}

static void appCreateObjects(QVariant resp)
{
    QJsonArray array = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
    for (auto row = array.begin(); row != array.end(); row++) {
        QJsonObject jsonRow=row->toObject();
        if (jsonRow.contains("DEVICE_TYPE")) {
            if (jsonRow["DEVICE_TYPE"].toString()=="SHERLOCK") {
                SherlockManager * sm = new SherlockManager;
                cfSetProperties(sm,jsonRow.toVariantMap());
            }
        }
    }
}

static void mcbLoadTcpDevices(const QVariant &resp)
{
//    qDebug()<<resp;
    //QJsonArray jsonArray = QJsonDocument::fromJson(resp.toString().toUtf8()).array();
    static const QByteArray fldClsName("CLASS_NAME");
    for (auto value:QJsonArray(QJsonDocument::fromJson(resp.toString().toUtf8()).array())) {
//    for (auto &value:jsonArray) { // not work
//    foreach (const QJsonValue &value, jsonArray) {
        QJsonObject jsonObject=value.toObject();
        if (jsonObject.contains(fldClsName)) {
            QString className(jsonObject[fldClsName].toString());
            if(className.isEmpty()){
                if (jsonObject.contains("DEVICE_NAME"))
                    qDebug() << QString("Type name of %1 is empty").arg(jsonObject["DEVICE_NAME"].toString());
                continue;
            }
            int id = QMetaType::type(className.toLatin1());
            if (id != 0) {
                const QMetaObject *meta_object = QMetaType::metaObjectForType(id);
                QObject* tcpDevice= meta_object->newInstance(Q_ARG(QObject*, qApp));
                cfSetProperties(tcpDevice,jsonObject.toVariantMap());
                qDebug() << QString("%1 %2 created")
                            .arg(tcpDevice->objectName()).arg(className).toLatin1(); //"Object \"%1\", type \"%2\" created"
            } else
                qDebug() << QString("Type ""%1"" not found").arg(className).toLatin1();
        }
    }
}


#endif // MAIN_CALLBACKS_H
