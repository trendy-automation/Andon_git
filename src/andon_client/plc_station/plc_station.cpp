#include "Plc_station.h"
#include <QDebug>

 // Callback shared between all partners
void S7API ParDataIncoming(void *usrPtr, int opResult, longword R_ID, void *pData, int Size)
{
 //    qDebug()<<"ParDataIncoming opResult"<<opResult << "R_ID" << R_ID;
    Plc_station *MyStation = (Plc_station *) usrPtr;
    if (opResult==0){
        QByteArray data= QByteArray((const char *)pData, Size);
        emit MyStation->reqDeclKanban(data,MyStation->getSapUser(R_ID),MyStation->getSapPass(R_ID),MyStation->getIdDevice());
    }
    else
        emit MyStation->error(opResult);
}

// Class implementation
Plc_station::Plc_station(QObject *parent) : QObject(parent)
{
    Partner = new TS7Partner(true);
    //Partner = Par_Create(1);
    Partner->SetRecvCallback(ParDataIncoming, this);

}

Plc_station::~Plc_station()
{
}

void Plc_station::Send(void *pData, int Size)
{
    Partner->BSend(1, pData, Size);
}

void Plc_station::StartTo(const QByteArray &LocalAddress, const QByteArray &RemoteAddress, word LocTsap, word RemTsap)
{
    Partner->StartTo(LocalAddress.constData(), RemoteAddress.constData(), LocTsap, RemTsap);
    qDebug()<<"Plc_station started" << this->objectName();

}

//void S7API Plc_station::RecvCallback(void *usrPtr, int opResult, longword R_ID, void *pData, int Size)
//{
//    Plc_station *MyStation = (Plc_station *) usrPtr;
//    if (opResult==0){
//        QByteArray data= QByteArray((const char *)pData, Size);
//        emit MyStation->reqDeclKanban(data,MyStation->getSapUser(R_ID),MyStation->getSapPass(R_ID));
//    }
//    else
//        emit MyStation->error(opResult);
//}

void Plc_station::resDeclKanban(int logKanbanId, const QByteArray &kanbanNumber, int result,int idTcpDevice, const QString &message)
{
//    qDebug()<<"resDeclKanban kanban declare finish by"<<idTcpDevice
//            <<(result==0?"OK":(QString("error ").append(QString().number(result))))
//            <<kanban<<message;
}
