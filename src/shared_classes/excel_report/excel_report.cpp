#include "excel_report.h"
#include <QtCore/QDebug>
#include "functional"
#include "common_functions.h"

#include "dbwrapper.h"
#include "sendemail.h"
//#include "qftp.h"

#include <QBuffer>
#include <QTimer>
//#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
//#include <QStringList>

#include "xlsxworkbook.h"
#include "xlsxworksheet.h"

ExcelReport::ExcelReport(QObject *parent) :
    QObject(parent)
{

}

ExcelReport::~ExcelReport()
{

}

bool ExcelReport::queryText2Document(const QString & queryText, Document *xlsx,
                                     const QString &sheetName, const QString &ariaName)
{
    DBWrapper *andonDb =cfGetObject<DBWrapper>("andonDb");
    if(!andonDb)
        return false;


//    QString res;
//    QMetaObject::invokeMethod(andonDb, "query2json", Qt::BlockingQueuedConnection,
//                              Q_RETURN_ARG(QString, res),
//                              Q_ARG(QString, queryText));

    QFuture<QString> future = QtConcurrent::run(andonDb,&DBWrapper::query2json,queryText);
    QTextCodec *codec = QTextCodec::codecForName("iso8859-1");
    int i=1;
    int field_count=1;
    for (auto row:QJsonDocument::fromJson(future.result().toUtf8()).array()){
        int j=1;
        QJsonObject clientsObject=row.toObject();
        if(i==1){
            field_count=clientsObject.keys().count();
            for(auto field_name:clientsObject.keys()){
                xlsx->write(i,j,QString(codec->fromUnicode(field_name)));
                j++;
            }
            j=1;
        }
        for(auto val:clientsObject){
            xlsx->write(i,j,val.toVariant());
            j++;
        }
    }
    if(!ariaName.isEmpty())
        if(!xlsx->defineName(ariaName,QString("='%1'!$A$1:$%2$%3").arg(sheetName)
                         .arg(QChar(QChar('A').unicode()+field_count-1)).arg(i)))
           qDebug()<<"Can not define aria name"<<ariaName;
    return true;
}

bool ExcelReport::queryText2Email(const QString & queryText, const QString &subject,
                                  const QStringList &rcptStringList, const QString &message,
                                  QString sheetName, QString fileName)
{
    if(sheetName.isEmpty())
        sheetName=subject;
    if(fileName.isEmpty())
        fileName=subject;
    Document * xlsx = new Document; // = newDocument(sheetName);
    if(!queryText2Document(queryText,xlsx,sheetName))
        return false;
    QBuffer *buffer = new QBuffer(new QByteArray);
    xlsx->saveAs(buffer);
    buffer->setProperty("FILE_NAME",fileName);
    SendEmail *emailClient =cfGetObject<SendEmail>("emailClient");
    if(!emailClient)
        return false;
    return emailClient->sendEmail(subject, message, rcptStringList, QList<QBuffer*>()<<buffer);
}

bool ExcelReport::queryText2File(const QString &queryText, const QString &sheetName,
                                 const QString &fileName,const QString &ariaName)
{
    Document *xlsx= new Document;
    if(xlsx->selectSheet(sheetName))
        xlsx->deleteSheet(sheetName);
    xlsx->addSheet(sheetName);
    if(!queryText2Document(queryText,xlsx,sheetName,ariaName))
        return false;
    if(xlsx->saveAs(fileName)){
        qDebug()<<fileName<<"save OK";
        return true;
    }
    else
        qDebug()<<fileName<<"not saved";
    return false;
}

void ExcelReport::addEmailReport(QTime baseTime, char unit, int interval,
                    const QString & queryText, const QString &subject,
                    const QStringList &rcptStringList, const QString &message,
                    const QString &sheetName,const QString &fileName)
{
    QTimer * t = new QTimer;
    QObject::connect(t,&QTimer::timeout,[=](){
        queryText2Email(queryText,subject,rcptStringList,message,sheetName,fileName);
        t->start(calcNextInterval(baseTime, unit, interval));
    });
    t->start(calcNextInterval(baseTime, unit, interval));
}

void ExcelReport::addFileReport(QTime baseTime, char unit, int interval,
                   const QString &queryText, const QString &sheetName,
                   const QString &fileName,const QString &ariaName)
{
    QTimer * t = new QTimer;
    QObject::connect(t,&QTimer::timeout,[=](){
        queryText2File(queryText,sheetName,fileName,ariaName);
        t->start(calcNextInterval(baseTime, unit, interval));
    });
    t->start(calcNextInterval(baseTime, unit, interval));
}

qint64 ExcelReport::calcNextInterval(QTime baseTime, char unit, int interval)
{
    QDateTime cdt(QDateTime::currentDateTime());
    QDateTime nextTime(cdt);
    if(unit==(uint)"M")
        nextTime.addMonths(interval);
    if(unit==(uint)"d")
        nextTime.addDays(interval);
    if(unit==(uint)"s")
        nextTime.addSecs(interval);
    nextTime.setTime(baseTime);
    qint64 result = cdt.msecsTo(nextTime);
    if(result<0)
        result = DEF_REPORT_INTERVAL;
    return result;
}
