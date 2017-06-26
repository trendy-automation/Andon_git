#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QTextStream>
#include <QTextCodec>
#include <QFile>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QAbstractEventDispatcher>
#include <QtMessageHandler>
#include <QMessageLogContext>
#include <QTimer>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <qapplication.h>

#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QFileInfo>




class MessageHandler: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QByteArray activeContextList READ getContextList)
    Q_PROPERTY(QByteArray logfileContextList READ getLogfileContextList WRITE setLogfileContextList)
    Q_PROPERTY(QByteArray consoleContextList READ getConsoleContextList WRITE setConsoleContextList)

public:
    MessageHandler(const QByteArray &cdcName="CP1251", const QString &flNameFormat = QString("DebugLog%1_yyyy_MMMM_dd.txt").arg(DEBUG_NAME),
                   const QString &prfxFormat = "HH:mm:ss.zzz")
        : QObject(qApp),
          logFile(new QFile),
          outStream(new QTextStream),
          codecName(cdcName),
          fileNameFormat(flNameFormat),
          prefixFormat(prfxFormat),
          logfileContextList(new QStringList),
          consoleContextList(new QStringList),
          activeContextList(new QStringList)
    {
        qDebug()<<"Start MessageHandler";
        QByteArray consoleCodecName;
        if(qApp->applicationDirPath().contains("build"))
            consoleCodecName="CP1251";
        else
            consoleCodecName="CP866";
        consoleCodec=QTextCodec::codecForName(consoleCodecName);
        createLogFile();
        outStream->setDevice(logFile);
//        QObject::connect(logFile, &QFile::aboutToClose, [this]{
//            qDebug() << "logFile aboutToClose" << logFile->errorString();
//        });
        //TODO monthly log


        currentMessageHandler = this;
        timerFN->setTimerType(Qt::VeryCoarseTimer);
//        QObject::connect(timerFN, &QTimer::timeout, this, &MessageHandler::createLogFile);
        QObject::connect(timerFN, &QTimer::timeout, logFile, &QFile::close);
        timerFN->start(msecsPerDay-QTime::currentTime().msecsSinceStartOfDay()+1000);
        qInstallMessageHandler(handleMessages);
        //TODO Numering of builds
        myMessageHandler(QtDebugMsg,QMessageLogContext(),
            "\r\n_________________________________________________________");
        myMessageHandler(QtDebugMsg,QMessageLogContext(),
                         QString("Application %1 ver.%2 build %3 runing").arg(DEBUG_NAME).arg(APP_VER)
                          .arg(QFileInfo(qApp->applicationFilePath()).created().toString("dd.MM.yy hh:mm")));
    }

    ~MessageHandler(){
        logFile->close();
        logFile->deleteLater();
        //timerFN->deleteLater();
    }

    void myMessageHandler(QtMsgType Type,const QMessageLogContext &Context,const QString &Message)
    {
        QString contextFile = QString(Context.file).split("\\").last().split("/").last();
        QString contextFunction = QString(Context.function).split("(").first().split(" ").last();
        QStringList msgList;
        msgList << QDateTime::currentDateTime().toString(prefixFormat);
        if ((!contextFile.isEmpty()) && (Context.line!=0))
            msgList << contextFunction.split("::").last() << QString::number(Context.line);
//            msgList << contextFile << contextFunction.split("::").last() << QString::number(Context.line);
        msgList << Message << "\n";
        QString msg=msgList.join(" ");
//        msg.replace("\"","""");
        if (!consoleContextList->contains(contextFile)){
            fprintf(stdout,consoleCodec->fromUnicode(msg));
            fflush(stdout);
        }
        if (!logfileContextList->contains(contextFile)){

//            fprintf(stdout,QString("logFile->isOpen()=%1\r\n").arg(logFile->isOpen()).toLatin1());
//            fflush(stdout);
            if(!logFile->isOpen())//{
                createLogFile();

//                fprintf(stdout,"logFile !logFile->isOpen()\r\n");
//                fflush(stdout);
//                if(logFile->open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append)){
//                    fprintf(stdout,"logFile OPENED!!!!!!!!!!!!!!\r\n");
//                    fflush(stdout);
//                }
//                else {
//                    fprintf(stdout,"Can not open logFile\r\n");
//                    fflush(stdout);
//                }
//            }
//            if(logFile->open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append)) {

                outStream->operator <<(msg.replace("\\\"","\""));
                //                logFile->close();
                //            }
                if(logFile->isOpen())
                    outStream->flush();
                else {
                        fprintf(stdout,consoleCodec->fromUnicode(
                                    QString("Message %1 not writed to file %2 because %3\r\n")
                                        .arg(msg).arg((((QFile*)outStream->device())->fileName()))
                                        .arg(logFile->errorString())));
                        fflush(stdout);
                    }
        }
        if(!activeContextList->contains(contextFile) && !contextFile.isEmpty())
            activeContextList->append(contextFile);
    }


    Q_INVOKABLE QByteArray getContextList() {
        return strLst2Json(activeContextList); }

    Q_INVOKABLE QByteArray getLogfileContextList() {
        qDebug() << strLst2ArrayVals(logfileContextList);
        return strLst2ArrayVals(logfileContextList); }

    Q_INVOKABLE QByteArray getConsoleContextList() {
        qDebug() << strLst2ArrayVals(consoleContextList);
        return strLst2ArrayVals(consoleContextList); }

    Q_INVOKABLE void setLogfileContextList(const QByteArray &logList) {
        qDebug() << logList;
        arrya2List(logfileContextList, logList);
    }

    Q_INVOKABLE void setConsoleContextList(const QByteArray &consoleList) {
        qDebug() << consoleList;
        arrya2List(consoleContextList, consoleList);
    }
private slots:
    void createLogFile() {
        if(logFile->isOpen())
            logFile->close();
        logFile->setFileName(QDate().currentDate().toString(fileNameFormat));
        fprintf(stdout,consoleCodec->fromUnicode(QString("Create log file %1\r\n")
                                                 .arg(logFile->fileName())));
        fflush(stdout);
        if(!logFile->open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Append))
            qDebug() << "File not opened"<<logFile->errorString();
        timerFN->stop();
        timerFN->start(msecsPerDay-QTime::currentTime().msecsSinceStartOfDay()+2000);
    }

private:
    static void handleMessages(QtMsgType Type,const QMessageLogContext &Context,const QString &Message) {
      if (currentMessageHandler)
        currentMessageHandler->myMessageHandler(Type, Context, Message);
    }

    const int msecsPerDay = 24 * 60 * 60 * 1000;
    //TODO different in different month
    const int msecsPerMonth = msecsPerDay*31;
    static MessageHandler *currentMessageHandler;
    QTextCodec *consoleCodec;
    QFile *logFile;
    QTextStream *outStream;
    QStringList *logfileContextList;
    QStringList *consoleContextList;
    QStringList *activeContextList;
    QByteArray codecName;
    QString fileNameFormat;
    QString prefixFormat;
    QTimer *timerFN = new QTimer(QAbstractEventDispatcher::instance());

    QByteArray strLst2Json(QStringList *stringList) {
        if(stringList) {
            QJsonArray jarrContext;
            for (int i=0;i<stringList->count();++i) {
                QJsonArray jarrTmp;
                jarrTmp.append(i);
                jarrTmp.append(QJsonValue::fromVariant(stringList->at(i)));
                jarrContext.append(QJsonValue::fromVariant(QJsonDocument(jarrTmp).toVariant()));
            }
            QJsonDocument jsonContext(jarrContext);
            //qDebug() << jsonContext.toJson(QJsonDocument::Compact);
            return jsonContext.toJson(QJsonDocument::Compact);
        }
        else
            return QByteArray();
    }
    QByteArray strLst2ArrayVals(QStringList *stringList) {
        if(stringList) {
            QJsonArray jarrContext;
            for (int i=0;i<stringList->count();++i) {
                if (activeContextList->contains(stringList->at(i)))
                    jarrContext.append(QJsonValue::fromVariant(
                                       activeContextList->indexOf(stringList->at(i))));
            }
            QJsonDocument jsonContext(jarrContext);
            //qDebug() << jsonContext.toJson(QJsonDocument::Compact);
            return jsonContext.toJson(QJsonDocument::Compact);
        }
        else
            return QByteArray();
    }
    void arrya2List(QStringList *stringList, const QByteArray &newList) {
        stringList->clear();
        QStringList newStringList = QString(newList).split(",");
        for (int i=0;i<newStringList.count();++i) {
            if (activeContextList->length()>newStringList.at(i).toInt())
                stringList->append(activeContextList->at(newStringList.at(i).toInt()));
        }
    }
};





#endif // MESSAGE_HANDLER_H
