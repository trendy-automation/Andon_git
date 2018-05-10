// Class definition 
#ifndef PLC_STATION_H
#define PLC_STATION_H

#include <QObject>
#include <QVariant>
#include <QDebug>
#include "snap7.h"

class Plc_station : public QObject
{
    Q_OBJECT
public:
    explicit Plc_station(QObject *parent = 0);
    ~Plc_station();
//    void S7API RecvCallback(void *usrPtr, int opResult, longword R_ID, void *pData, int Size);
    void Send(void *pData, int Size);
    void StartTo(const QByteArray &LocalAddress, const QByteArray &RemoteAddress, word LocTsap, word RemTsap);
    void setIdDevice(int idTcpDevice)
    { idDevice = idTcpDevice; }
    int getIdDevice()
    { return idDevice; }
    void setUsers(const QVariantList &usersList)
    { users = usersList; }
    QByteArray getSapPass(int R_ID)
    { if(users.length()>(R_ID*2-1)) return users.at(R_ID*2-1).toByteArray();
        else return ""; }
    QByteArray getSapUser(int R_ID)
    { if(users.length()>(R_ID*2-1)) return users.at(R_ID*2-2).toByteArray();
        else return ""; }
    int getStatus()
    { return Partner->Status();}
//private:
    TS7Partner *Partner;
signals:
    dataReceived(const QByteArray &data);
    reqDeclKanban(int logKanbanId, const QByteArray &kanbanNumber, const QByteArray &user, const QByteArray &pass, int idDevice);
    error(int opResult);
public slots:
    void resDeclKanban(int logKanbanId, const QByteArray &kanbanNumber, int result, int idTcpDevice, const QString &message);
    void kanbanDeclError(const QByteArray &kanbanNumber);
private:
    QVariantList users;
    int idDevice;
};

#endif
