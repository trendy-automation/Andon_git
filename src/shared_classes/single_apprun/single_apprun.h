#ifndef SINGLE_APPRUN_H
#define SINGLE_APPRUN_H

#include <QObject>
#include <QMessageBox>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QProcess>
#include <QHostAddress>
#include <QTimer>
#include <QThread>

class SingleAppRun: public QObject
{
    Q_OBJECT
public:
    SingleAppRun(bool forceStart=false,QObject *parent=0)
        : QObject(parent),
          shmem(new QSharedMemory(parent)),
          toQuit(false)
    {
        QString pid= QString::number(qApp->applicationPid());
        qDebug()<<"Application pid"<<pid;
        if(isRunning(QString("<ANDON %1 VER%2>").arg(APP_NAME).arg(APP_VER),
                     QString("<ANDON %1 VER%2 RUNNING>").arg(APP_NAME).arg(APP_VER), pid)) {
            qDebug() << QString("%1 is already running!").arg(APP_NAME);
            if(forceStart){
                qDebug() << QString("%1 forced restart!").arg(APP_NAME);
                killProcess(pid);
                QThread::sleep(3);
            }
            else {
                int doTerminate=QMessageBox::question(new QWidget, QString("%1 is already running").arg(APP_NAME),
                                                      "Terminate concurent application?");
                switch (doTerminate) {
                case QMessageBox::Yes:
                {
                    killProcess(pid);
                    break;
                }
                case QMessageBox::No:
                {
                    toQuit=true;
                    break;
                }
                default:
                    qDebug()<<"doTerminate"<<doTerminate;
                    qApp->quit();
                    break;
                }
            }
        }
    }

    bool isToQuit(){return toQuit;}

private:
    bool isRunning(const QString &semaphore, const QString &memory, QString &pid)
    {
        QSystemSemaphore sema(semaphore, 1);
        sema.acquire();
        {
            QSharedMemory shmem2(memory);
            shmem2.attach();
        }
        shmem->setKey(memory);
        if (shmem->attach()){
            shmem->lock();
            pid = QString((char*)shmem->data());
            shmem->unlock();
            return true;
        }
        shmem->create(pid.size());
        shmem->lock();
        memcpy((char*)shmem->data(), (char *)pid.toLatin1().data(), pid.size());
        shmem->unlock();
        sema.release();
        return false;
    }

    void killProcess(const QString &pid)
    {
        shmem->lock();
        QString new_pid = QString::number(qApp->applicationPid());
        memcpy((char*)shmem->data(), (char *)new_pid.toLatin1().data(), qMax(shmem->size(), new_pid.size()));
        shmem->unlock();
        QString command = QString("taskkill /T /F /PID %1").arg(pid);
        qDebug()<<command<<". Application pid"<<qApp->applicationPid();
        QProcess * processKill = new QProcess;
        processKill->startDetached(command);
    }

    QSharedMemory *shmem;
    bool toQuit;
};

#endif // SINGLE_APPRUN_H
