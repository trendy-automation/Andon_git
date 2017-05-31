#ifndef WEBUITHEARD_H
#define WEBUITHEARD_H

#include <QThread>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QTextCodec>

//_______Excel Lib_______________
//#include "xlsxdocument.h"
#include <QIODevice>
#include <QDataStream>
#include <QBuffer>
#include <QSqlQuery>
#include <QSqlRecord>


#include "mongoose\mongoose.h"
#include <string.h>

//using namespace std;
#include <functional>
#include <QHostAddress>
#include <QTcpSocket>


//#include <QtScript>
#include <QJSEngine>
#include <QApplication>


struct pageInfo
{QString pageData;
    QTime pageTime;};

static struct mg_serve_http_opts s_http_server_opts;

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
    if (ev == MG_EV_HTTP_REQUEST) {
        mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
    }
}

//static int event_handler(struct mg_connection *conn, enum mg_event ev)
//{
//if (ev == MG_AUTH)
//return MG_TRUE;   // Authorize all requests
//else if (ev == MG_REQUEST)
////  qDebug() << "REQUEST" << conn->uri
////       << "from" << conn->remote_ip
////       << ":" << conn->remote_port;
//if (strcmp(conn->uri, "/graff")==0) {
//    //qDebug() << "mg_send_file(conn,graff.html)";
//    mg_send_file(conn,"./webui/graff.html","");
//    return MG_MORE;
//}
//return MG_FALSE;  // Rest of the events are not processed
//}


class WebuiThread : public QThread
{
    Q_OBJECT
public:
    WebuiThread(QObject *parent=0)    : QThread(parent)
    {
        QByteArray document_root = (qApp->applicationDirPath().contains("build")>0)?WUI_BUILD_PATH:WUI_PATH;
        //qDebug() << "document_root" << document_root;
        //mgserver = mg_create_server(NULL, event_handler);
        //mg_set_option(mgserver, "document_root", document_root);
        s_http_server_opts.document_root = document_root;  // Serve current directory
        s_http_server_opts.enable_directory_listing = "yes";

        QObject::connect(&cleanTimer, &QTimer::timeout,[=](){
            for(auto iter = webuiPages.begin();iter!=webuiPages.end();)
                if (iter.value().pageTime.elapsed()>cleanTimer.interval()) {
                    iter=webuiPages.erase(iter);
                } else ++iter;
        });
        cleanTimer.start(WUI_CLAEN_INTERVAL);
    }

    void setEngine(QJSEngine* SharedEngine)
    {
        engine=SharedEngine;
    }
signals:
    void sendText(const QString &sql_query,const QString &query_method);
    void getSqlQuery(const QString &sql_query,const QString &query_method,
                     std::function<void(QString)> functor=[](QString jsontext){});
    //void getSqlQuery(const QString &sql_query,
    //             std::function<void(QSqlQuery /***/query)> functor=[](QSqlQuery /***/query){});

    void sendReport2email(const QString &subject, const QString &message,
                          const QStringList &rcptStringList, QList<QBuffer*> *attachments=0);


public slots:
    QHostAddress serverAddress() const
    {
        return QHostAddress::LocalHost;
    }

    bool listen(QHostAddress address = QHostAddress::AnyIPv4, quint16 port = WUI_PORT)
    {
        //address=QHostAddress::Any;
//        QTcpSocket tmpSock;
//        if(tmpSock.bind(port)) {
//            tmpSock.close();
            qDebug()<<1;
            std::string buf = "";
            char s_http_port[5];
            buf += itoa(port, s_http_port, 10);
            qDebug()<<2;
            //    mg_set_option(mgserver, "listening_port", port_char);


            if(!mgr)
                mgr = new mg_mgr;
            qDebug()<<3;
            mg_mgr_init(mgr, NULL);
            qDebug()<<4;
            nc = mg_bind(mgr, s_http_port, ev_handler);
            qDebug()<<5;
            if (nc == NULL)
                return false;
            qDebug()<<6;
            // Set up HTTP server parameters
            mg_set_protocol_http_websocket(nc);
            qDebug()<<7;
            return true;
//        }
//        else
//            return false;
    }

    /*    void updateWebuiData(const QString &sql_query, const QString &jsontext)
    {
        webuiPages.insert(sql_query,{jsontext,QTime::currentTime()});
        qDebug() << "sql_query"<<sql_query;
//         << webuiPages.count()<<update_count;
        emit sendText(sql_query, jsontext);
//if (update_count<932838457459459)
//    update_count++;
    }
  */

    void receiveText(const QString &query, const QString &query_method)
    {
        //qDebug() << "sql_query"<<sql_query;
        //         << "query_method" << query_method << "webuiPages.count()" << webuiPages.count()
        //         << "recive_count" << recive_count;
        QString sql_query=QString(query).toUpper();
        if (webuiPages.contains(sql_query)) {
            if (webuiPages.value(sql_query).pageTime.elapsed()<WUI_UPDATE_INTERVAL) {
                //        qDebug() << "webuiPages.contains(sql_query)";
                //                 << webuiPages.count()<<recive_count;
                if (!webuiPages.value(sql_query).pageData.isEmpty()){
                    //            qDebug() << "emit sendText";
                    emit sendText(sql_query, webuiPages.value(sql_query).pageData);
                    return;
                }
                else {
                    qDebug() << "pageData.isEmpty()";
                }
            }
        } else {
            webuiPages.insert(sql_query,{QString(),QTime::currentTime()});
        }
        getSqlQuery(sql_query, query_method, [this, sql_query](const QString &jsontext){
            webuiPages.insert(sql_query,{jsontext,QTime::currentTime()});
            emit sendText(sql_query, jsontext);
        });

    }
    void snedReport(const QString &report, const QStringList &emails)
    {
        QString sql_query;
        QString template_file;
        QString res_file="report.xlsx";
        QString subject;
        QString sheet;
        //SELECT p.DESCRIPTION
        //FROM DB_REPORT_DESCRIPTION(PROCEDURE_NAME) p
        if (report==QString("Daily_PDP")) {
            sql_query=QString("SELECT * FROM PRODUCTION_REPORT_SHIFT_PDP");
            res_file=QString("PDP %1.xlsx").arg(QDate::currentDate().toString("ddd d MMMM"));
            subject=QString("Производственный отчёт %1 за %2 смену ")
                    .arg(QDate::currentDate().toString("ddd d MMMM")).arg(QTime::currentTime().hour()<15?1:2);
        }
        if (report==QString("Weekly_PDP")) {
            sql_query="SELECT * FROM PRODUCTION_REPORT_PDP";
            template_file="PDP.xlsx";
            res_file=QString("PDP W%1.xlsx").arg(QDate::currentDate().weekNumber());
            subject=QString("Производственный отчёт большие тримы за неделю %1").arg(QDate::currentDate().weekNumber()) ;
        }
        if (report==QString("REPORT_BREAKDOWNS")) {
            sql_query="SELECT * FROM REPORT_BREAKDOWNS";
            res_file=QString("REPORT_BREAKDOWNS_%1.xlsx").arg(QDate::currentDate().toString("dd_MM_yyyy"));
            subject=QString("Простои производства %1").arg(QDate::currentDate().toString("ddd d MMMM"));
            sheet=QString("Отчёт по простоям %1").arg(QDate::currentDate().toString("ddd d MMMM"));
        }

        //qDebug()<<"getSqlQuery"<<report<<emails<<sql_query;
        //getSqlQuery(sql_query, [this,&subject,&sheet,&template_file,&res_file,emails]
        //                (QSqlQuery /***/query){
        //        QXlsx::Document * xlsx;
        //        if (template_file.isEmpty())
        //            xlsx = new QXlsx::Document();
        //        else
        //            xlsx = new QXlsx::Document(template_file);
        //        xlsx->addSheet(sheet);
        //        int i=1;
        //        QTextCodec *codec = QTextCodec::codecForName("iso8859-1");
        //        for(int j=0; j < query->record().count(); j++)
        //            xlsx->write(i,j+1,QString(codec->fromUnicode(query->record().fieldName(j))));
        //        while(query->next()) {
        //            i++;
        //            for(int j=0; j < query->record().count(); j++)
        //                xlsx->write(i,j+1,query->value(j));
        //        }
        //        if(i>1){
        //            QBuffer buffer(new QByteArray);
        //            xlsx->saveAs(&buffer);
        //            buffer.setProperty("FILE_NAME",res_file);
        //            emit sendReport2email(subject,"",emails,&(QList<QBuffer*>()<<&buffer));
        //        }
        //    });
    }


private:
    QJSEngine *engine;
    QMap<QString,pageInfo> webuiPages;
    QTimer cleanTimer;
    //struct mg_server *mgserver;
    struct mg_mgr *mgr;
    struct mg_connection *nc;

    //qint64 update_count=0;
    //qint64 recive_count=0;

protected:
    virtual void run(void)
    {
        //for (;;) {
        //    mg_poll_server(mgserver, 1000);   // Infinite loop, Ctrl-C to stop
        //}
        //mg_destroy_server(&mgserver);
        for (;;) {
            if(mgr)
                mg_mgr_poll(mgr, 1000);
        }
        mg_mgr_free(mgr);

    }
};

#endif // WEBUITHEARD_H
