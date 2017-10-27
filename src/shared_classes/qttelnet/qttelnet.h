/****************************************************************************
**
** This file is part of a Qt Solutions component.
** 
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
** 
** Contact:  Qt Software Information (qt-info@nokia.com)
** 
** Commercial Usage  
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
** 
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
** 
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
** 
** GNU General Public License Usage 
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** 
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
** 
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** 
****************************************************************************/

#ifndef QTTELNET_H
#define QTTELNET_H

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QSize>
#include <QtCore/QRegExp>
//#include <QtCore/QRegularExpression>
#include <QQueue>
#include <QTimer>
#include <QtNetwork/QTcpSocket>

class QtTelnetPrivate;

#if defined(Q_WS_WIN)
#  if !defined(QT_QTTELNET_EXPORT) && !defined(QT_QTTELNET_IMPORT)
#    define QT_QTTELNET_EXPORT
#  elif defined(QT_QTTELNET_IMPORT)
#    if defined(QT_QTTELNET_EXPORT)
#      undef QT_QTTELNET_EXPORT
#    endif
#    define QT_QTTELNET_EXPORT __declspec(dllimport)
#  elif defined(QT_QTTELNET_EXPORT)
#    undef QT_QTTELNET_EXPORT
#    define QT_QTTELNET_EXPORT __declspec(dllexport)
#  endif
#else
#  define QT_QTTELNET_EXPORT
#endif

class QT_QTTELNET_EXPORT QtTelnet : public QObject/*QThread*/
{
    Q_OBJECT
    friend class QtTelnetPrivate;
public:
    QtTelnet(QObject *parent = 0); //QThread *thread = 0,
    ~QtTelnet();

    enum Control            { GoAhead, InterruptProcess, AreYouThere, AbortOutput,
                              EraseCharacter, EraseLine, Break, EndOfFile, Suspend,
                              Abort };

    enum TerminalState      { trmUnknown, trmTelnetDisconnected, trmSocketDisconnected, trmTelnetConnecting,
                              trmSapConnecting, trmSapKanbanMenu, trmSapKanbanProcessing,
                              trmBackToKanbanMenu};
    Q_ENUM(TerminalState)
    enum ProtocolError      { errNoError, errSapLogin, errTelnetLoginFailed, errTaskTimeout,
                              errSapNotPosting, errSapLoginError, errKanbanMessage, errNoKanban,
                              errKanbanNoProdVer, errKanbanInLoop, errServerBrokeTheConnection,
                              errCloseConnection, errKanbanNotDefined
                            };
    Q_ENUM(ProtocolError)
    enum ProtocolInfo       { infNoInfo, infSapUserNoPrinter, infTaskFinished, infNoPrinter,
                              infKanbanDeclSuccess, infKanbanErrorDataLock, infLogOff, infSapStatusInfo
                            };
    Q_ENUM(ProtocolInfo)
    enum DlgState           { dlgKanbanDeclSuccess, dlgSessionClosed, dlgConnectionBroken,
                              dlgLogonTerminated, dlgLoggedIn, dlgLoginRequired, dlgPassRequired, dlgIntro,
                              dlgPassStars, dlgProcessing, dlgPassPolicy, dlgLoginProccessing,
                              dlgSapIntro, dlgSapProcessing, dlgSapNotPosting, dlgSapLoggedIn,
                              dlgSapMenu, dlgProdDeclMenu, dlgDeclKanbanMenu, dlgDeclKanbanOld,
                              dlgSapErrorKanbanNoProdVer, dlgSapErrorMessage, dlgDeclKanbanFail,
                              dlgSapKanbanDataLock, dlgSapErrorKanbanNoPrinter, dlgSapErrorNoKanban,
                              dlgLoginFailed, dlgSapLoginError, dlgWantLogoff, dlgSoketDisconnected,
                              dlgKanbanInLoop, dlgKanbanNoPrinter, dlgBackToKanbanMenu, dlgSapStatusInfo
                            };
    Q_ENUM(DlgState)


//    enum SapTrans   { ZJPP07, nonTrans };

    struct Task {
        int                     logKanbanId;
        QByteArray              kanbanNumber;
        QByteArray              user;
        QByteArray              pass;
        int                     idDevice;
    };

    struct State {
        TerminalState           trmState;
        QList<ProtocolError>    errState;
        QList<ProtocolInfo>     infState;
        QList<DlgState>         dlgTracking;
        Task                    activeTask;
        QString                 lastMessage;
    };

    void start();
    void connectToHost(const QString &host="", quint16 port = 23);

    void setWindowSize(const QSize &size);
    void setWindowSize(int width, int height); // In number of characters
    QSize windowSize() const;
    bool isValidWindowSize() const;

    void setSocket(QTcpSocket *socket);
    QTcpSocket *socket() const;

    void setPromptPattern(const QRegExp &pattern);
    void setPromptString(const QString &pattern)
    { setPromptPattern(QRegExp(QRegExp::escape(pattern))); }
//public:
    void setLoginPattern(const QRegExp &pattern);
    void setLoginString(const QString &pattern)
    { setLoginPattern(QRegExp(QRegExp::escape(pattern))); }
    void setPasswordPattern(const QRegExp &pattern);
    void setPasswordString(const QString &pattern)
    { setPasswordPattern(QRegExp(QRegExp::escape(pattern))); }
    void setHost(const QString &host)
    { host_addres=host; }
    void setTelnetUser(const QString &user)
    { telnet_user=user; }
    void setTelnetPass(const QString &pass)
    { telnet_pass=pass; }

public slots:
    void login(const QString &user="", const QString &pass="");
    void logout();
    void close();
    void sendControl(Control ctrl);
    void sendData(const QString &data);
//    void sendStr(const QString &str);
//    http://www.novell.com/documentation/integrationmanager6/Docs/help/Composer/books/TelnetAppendixB.html
    void sendSync();
//    void executeTransaction(SapTrans trans);
    void setDefaultUser(const QByteArray &user, const QByteArray &pass);
    void kanbanDeclare(int logKanbanId, const QByteArray &kanban, QByteArray user="", QByteArray pass="", int idDevice=0);
    void setTaskTimeout(int timeout);

signals:
    void serverBrokeTheConnection();
    void sendString(const QString &str);
    void connected();
    void disconnected();
    void loginRequired();
    void loginFailed();
    void loggedIn();
    void loggedOut();
    void connectionError(QAbstractSocket::SocketError error);
    void message(const QByteArray &data);
    void stateChanged(DlgState state, int key=0, QStringList capTexts=QStringList());
    void kanbanFinished(int logKanbanId, const QByteArray &kanban, int error, int idDevice, const QString &message="");
    void taskStarted();
    void taskRestarted(int delay);

private slots:
    bool parseMessage(const QByteArray &data);
    void goNextStep(DlgState state, int key=-1, QStringList capTexts=QStringList());
    void kanbanFinish(ProtocolError error, const QString &capTexts="");

private:
    void taskRunNext();
    void taskStart();
    void taskFinish();
    QByteArray stripCR(const QByteArray &msg);
    void patternInsert(DlgState dState, const QByteArray &pattern);



    QtTelnetPrivate                      *d;
    QMultiMap<DlgState,QRegExp>          patternMap;
    QMap<DlgState, TerminalState>        trmStateMap;
    QMultiMap<DlgState, ProtocolError>   errStateMap;
    QMultiMap<DlgState, ProtocolInfo>    infStateMap;
    QQueue<Task>                         taskQueue;
    QList<QByteArray>                    parseBuff;
    QByteArray                           defaultSapUser=SAP_DEF_LOGIN;
    QByteArray                           defaultSapPass=SAP_DEF_PASS;
    State                                currentState={trmTelnetDisconnected,QList<ProtocolError>()<<errNoError,
                                                       QList<ProtocolInfo>()<<infNoInfo,QList<DlgState>(),{0,"","","",0},QString()};
    Task                                 emptyTask={0,"","","",0};
    QTimer                               taskTimer;
    QTimer                               delayTaskTimer;
    int                                  taskTimeout=DEF_TASK_TIMEOUT;
    QString                              host_addres=TELNET_HOST;
    QString                              telnet_user=TELNET_LOGIN;
    QString                              telnet_pass=TELNET_PASS;



//protected:
//    virtual void run(void);

};
#endif
