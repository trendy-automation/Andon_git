#ifndef DBWRAPPER_H
#define DBWRAPPER_H

#include <QObject>
#include <QtSql>
//#include <QSqlError>
#include <QDateTime>
#include <QTimer>
#include <QJSEngine>
#include <functional>



//#include "ibpp.h"
//using namespace std;
struct queryStruct{
    QSqlQuery* p_query;
    QString s_key;
    QString s_sql_query;
    QString s_method;
    QString s_result;
    QString s_error;
    int i_cashTime; //ms
    QDateTime t_time;
};

class DBWrapper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int queryCount READ getQueryCount NOTIFY querysChanged)

public:
    explicit DBWrapper(QObject *parent = 0);
    ~DBWrapper();
    int getQueryCount(){return queryMap.count();}

signals:
    void DBConnected();
    void querysChanged();
    void resultQuery(const QString &sql_query, const QString &jsontext);
    void sendText(const QString &sql_query,const QString &query_method);
    void dbError(const QString &lastError);

public slots:
    QString query2method(const QString & queryText, const QString &queryMethod, int cashTime=0);
//    QString query2fulljson(const QString & queryText, int cashTime);
//    QString query2jsonstrlist(const QString & queryText, int cashTime);
    QString query2cash(const QString & queryText, int cashTime);
    QString query2fulljson(const QString & queryText);
    QString query2jsonstrlist(const QString & queryText);
    QString query2json(const QString & queryText);
    //QString query2map(const QString & queryText);
    bool ConnectDB(const QString &DB_Path,const QString &DB_Name);
    void executeQuery(const QString & queryText, const QString &query_method,
                      std::function<void(QString)> functor=[] (QString jsontext) {});
//    void executeQuery(const QString & queryText,
//                      std::function<void(QSqlQuery)> functor=[] (QSqlQuery /***/query) {});
    bool executeProc(const QString & queryText);
    QSqlQuery /***/sql2Query(const QString & queryText);

    bool getDbState();

private:
    queryStruct appendQuery(const QString &queryText, const QString &queryMethod, int cashTime);
    bool queryIsCashed(queryStruct &queryItem);
    bool queryExecute(queryStruct &queryItem);

    queryStruct emptyQuery = {0, QString(), QString(), QString(), QString(), QString(), 0, QDateTime()};
    QString errorStr;
    QJSEngine *engine;
    //TODO pointers of queryStruct in queryMap
    //QMap<QString,*queryStruct> queryMap;
    QMap<QString,queryStruct> queryMap;
    QMap<QString,std::function<QString(QSqlQuery*sqlQuery)>> packFunctionsMap;
    QString getQueryLastError(QSqlQuery *query);
    QString str2Json(const QString & name, const QString &val);
    QSqlDatabase DB;
    const QString queryKeyMask = "%1=>%2";
    int errorCounter=0;
    int signalErrorCount=20;
    int dbOK=0;

};

#endif // DBWRAPPER_H
