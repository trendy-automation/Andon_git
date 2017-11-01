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

#include <QtCore/QDebug>


/*!
    \class QtTelnet
    \brief The QtTelnet class proveds an API to connect to Telnet servers,
    issue commands and receive replies.

    When a QtTelnet object has been created, you need to call
    connectToHost() to establish a connection with a Telnet server.
    When the connection is established the connected() signal is
    emitted. At this point you should call login(). The
    QtTelnet object will emit connectionError() if the connection
    fails, and authenticationFailed() if the login() failed.

    Once the connection has been successfully established and
    you've logged in you can send control messages using sendControl()
    and data using sendData(). Connect to the message() signal to
    receive data from the Telnet server. The connection is closed with
    close().

    You can use your own socket if you call setSocket() before
    connecting. The socket used by QtTelnet is available from
    socket().
*/

#include "qttelnet.h"
#include <QtNetwork/QTcpSocket>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QVariant>
#include <QtCore/QSocketNotifier>
#include <QtCore/QBuffer>
#include <QtCore/QVarLengthArray>
#include <QtEndian>
#include <QMetaEnum>

//#  include <winsock.h>
//#  include <winsock2.h>

#if defined (Q_OS_WIN)
//#  include <winsock.h>
#  include <winsock2.h>

#endif
#if defined (Q_OS_UNIX)
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#endif

// #define QTTELNET_DEBUG

#ifdef QTTELNET_DEBUG
#include <QtCore/QDebug>
#endif

namespace FuncKeys
{
    // Functional keys
    QByteArray F1 = "\x1bOP";
    QByteArray F2 = "\x1bOQ";
    QByteArray F3 = "\x1bOR";
    QByteArray F4 = "\x1bOS";
    QByteArray F5 = "\x1b[15~";
    QByteArray F6 = "\x1b[17~";
    QByteArray F7 = "\x1b[18~";
    QByteArray F8 = "\x1b[19~";
    QByteArray F9 = "\x1b[20~";
    QByteArray F10 = "\x1b[21~";
    QByteArray F11 = "\x1b[23~";
    QByteArray F12 = "\x1b[24~";
    QByteArray F13 = "\x1b[25~";
    QByteArray F14 = "\x1b[26~";
    QByteArray F15 = "\x1b[28~";
    QByteArray F16 = "\x1b[29~";
    QByteArray F17 = "\x1b[31~";
    QByteArray F18 = "\x1b[32~";
    QByteArray F19 = "\x1b[33~";
    QByteArray F20 = "\x1b[34~";
}

namespace Telnet_Control
{
    QByteArray AUTO_WRAP_OFF  = "\x1b[?7l";
    QByteArray AUTO_WRAP_ON   = "\x1b[?7h";
    QByteArray CLEAR_SCREEN	  = "\x1b[2J";
    QByteArray CURSOR_OFF     = "\x1b[?25l";
    QByteArray CURSOR_ON      = "\x1b[?25h";
    QByteArray MOVE           = "\x1b[";
    QByteArray UP_ARROW       = "\x1b[A";
    QByteArray DOWN_ARROW     = "\x1b[B";
}
/*
QByteArray ANSI_BG_DEFAULT_COLOR = "\x1b[49m";
QByteArray ANSI_BLACK = "\x1b[30m";
QByteArray ANSI_BLACK_BG = "\x1b[40m";
QByteArray ANSI_BLINK_FAST = "\x1b[6m";
QByteArray ANSI_BLINK_SLOW = "\x1b[5m";
QByteArray ANSI_BLUE = "\x1b[34m";
QByteArray ANSI_BLUE_BG = "\x1b[44m";
QByteArray ANSI_BOLD = "\x1b[1m";
QByteArray ANSI_CLEAR_LINE_AFTER = "\x1b[0K";
QByteArray ANSI_CLEAR_LINE_ALL = "\x1b[2K";
QByteArray ANSI_CLEAR_LINE_BEFORE = "\x1b[1K";
QByteArray ANSI_CLEAR_SCREEN_AFTER = "\x1b[0J";
QByteArray ANSI_CLEAR_SCREEN_ALL = "\x1b[2J";
QByteArray ANSI_CLEAR_SCREEN_BEFORE = "\x1b[1J";
QByteArray ANSI_CURSOR_BACK = "\x1b[1D";
QByteArray ANSI_CURSOR_DOWN = "\x1b[1B";
QByteArray ANSI_CURSOR_FORWARD = "\x1b[1C";
QByteArray ANSI_CURSOR_HIDE = "\x1b[?25l";
QByteArray ANSI_CURSOR_MOVE_TO = "\x1b[3;3H";
QByteArray ANSI_CURSOR_NEXT_LINE = "\x1b[1E";
QByteArray ANSI_CURSOR_PREV_LINE = "\x1b[1F";
QByteArray ANSI_CURSOR_RESTORE_POS = "\x1b[u";
QByteArray ANSI_CURSOR_SAVE_POS = "\x1b[s";
QByteArray ANSI_CURSOR_SHOW = "\x1b[?25h";
QByteArray ANSI_CURSOR_UP = "\x1b[1A";
QByteArray ANSI_CYAN = "\x1b[36m";
QByteArray ANSI_CYAN_BG = "\x1b[46m";
QByteArray ANSI_DOWN = 18;
QByteArray ANSI_FAINT = "\x1b[2m";
QByteArray ANSI_GREEN = "\x1b[32m";
QByteArray ANSI_GREEN_BG = "\x1b[42m";
QByteArray ANSI_ITALIC = "\x1b[3m";
QByteArray ANSI_LEFT = 19;
QByteArray ANSI_NEGATIVE = "\x1b[7m";
QByteArray ANSI_PURPLE = "\x1b[35m";
QByteArray ANSI_PURPLE_BG = "\x1b[45m";
QByteArray ANSI_RED = "\x1b[31m";
QByteArray ANSI_RED_BG = "\x1b[41m";
QByteArray ANSI_RESET = "\x1b[0m";
QByteArray ANSI_RIGHT = 20;
QByteArray ANSI_SCROLL_DOWN = "\x1b[1T";
QByteArray ANSI_SCROLL_UP = "\x1b[1S";
QByteArray ANSI_TEXT_DEFAULT_COLOR = "\x1b[39m";
QByteArray ANSI_UNDERLINE = "\x1b[4m";
QByteArray ANSI_UP = 17;
QByteArray ANSI_WHITE = "\x1b[37m";
QByteArray ANSI_WHITE_BG = "\x1b[47m";
QByteArray ANSI_YELLOW = "\x1b[33m";
QByteArray ANSI_YELLOW_BG = "\x1b[43m";
*/

class QtTelnetAuth
{
public:
    enum State { AuthIntermediate, AuthSuccess, AuthFailure };

    QtTelnetAuth(char code) : st(AuthIntermediate), cd(code) {};
    virtual ~QtTelnetAuth() {}

    int code() const { return cd; }
    State state() const { return st; }
    void setState(State state) { st = state; };

    virtual QByteArray authStep(const QByteArray &data) = 0;

private:
    State st;
    int   cd;
};

class QtTelnetReceiveBuffer
{
public:
    QtTelnetReceiveBuffer() : bytesAvailable(0) {}
    void append(const QByteArray &data) { buffers.append(data); }
    void push_back(const QByteArray &data) { buffers.prepend(data); }
    long size() const { return bytesAvailable; }
    QByteArray readAll()
    {
        QByteArray a;
        while (!buffers.isEmpty()) {
            a.append(buffers.takeFirst());
        }
        return a;
    }

private:
    QList<QByteArray> buffers;
    long bytesAvailable;
};

namespace Common // RFC854
{
    // Commands
    const uchar CEOF  = 236;
    const uchar SUSP  = 237;
    const uchar ABORT = 238;
    const uchar SE    = 240;
    const uchar NOP   = 241;
    const uchar DM    = 242;
    const uchar BRK   = 243;
    const uchar IP    = 244;
    const uchar AO    = 245;
    const uchar AYT   = 246;
    const uchar EC    = 247;
    const uchar EL    = 248;
    const uchar GA    = 249;
    const uchar SB    = 250;
    const uchar WILL  = 251;
    const uchar WONT  = 252;
    const uchar DO    = 253;
    const uchar DONT  = 254;
    const uchar IAC   = 255;

    // Types
    const char IS    = 0;
    const char SEND  = 1;

    const char Authentication = 37; // RFC1416,
                                    // implemented to always return NULL
    const char SuppressGoAhead = 3; // RFC858
    const char Echo = 1; // RFC857, not implemented (returns WONT/DONT)
    const char LineMode = 34; // RFC1184, implemented
    const uchar LineModeEOF = 236, // RFC1184, not implemented
                LineModeSUSP = 237,
                LineModeABORT = 238;
    const char Status = 5; // RFC859, should be implemented!
    const char Logout = 18; // RFC727, implemented
    const char TerminalType = 24; // RFC1091,
                                  // implemented to always return UNKNOWN
    const char NAWS = 31; // RFC1073, implemented
    const char TerminalSpeed = 32; // RFC1079, not implemented
    const char FlowControl = 33; // RFC1372, should be implemented?
    const char XDisplayLocation = 35; // RFC1096, not implemented
    const char EnvironmentOld = 36; // RFC1408, should not be implemented!
    const char Environment = 39; // RFC1572, should be implemented
    const char Encrypt = 38; // RFC2946, not implemented

#ifdef QTTELNET_DEBUG
    QString typeStr(char op)
    {
        QString str;
        switch (op) {
        case IS:
            str = "IS";
            break;
        case SEND:
            str = "SEND";
            break;
        default:
            str = QString("Unknown common type (%1)").arg(op);
        }
        return str;
    }
    QString operationStr(char op)
    {
        QString str;
        switch (quint8(op)) {
        case quint8(WILL):
            str = "WILL";
            break;
        case quint8(WONT):
            str = "WONT";
            break;
        case quint8(DO):
            str = "DO";
            break;
        case quint8(DONT):
            str = "DONT";
            break;
        case quint8(SB):
            str = "SB";
            break;
        default:
            str = QString("Unknown operation (%1)").arg(quint8(op));
        }
        return str;
    }

    QString optionStr(char op)
    {
        QString str;
        switch (op) {
        case Authentication:
            str = "AUTHENTICATION";
            break;
        case SuppressGoAhead:
            str = "SUPPRESS GO AHEAD";
            break;
        case Echo:
            str = "ECHO";
            break;
        case LineMode:
            str = "LINEMODE";
            break;
        case Status:
            str = "STATUS";
            break;
        case Logout:
            str = "LOGOUT";
            break;
        case TerminalType:
            str = "TERMINAL-TYPE";
            break;
        case TerminalSpeed:
            str = "TERMINAL-SPEED";
            break;
        case NAWS:
            str = "NAWS";
            break;
        case FlowControl:
            str = "TOGGLE-FLOW-CONTROL";
            break;
        case XDisplayLocation:
            str = "X-DISPLAY-LOCATION";
            break;
        case EnvironmentOld:
            str = "ENVIRON";
            break;
        case Environment:
            str = "NEW-ENVIRON";
            break;
        case Encrypt:
            str = "ENCRYPT";
            break;
        default:
            str = QString("Unknown option (%1)").arg(op);
        }
        return str;
    }
#endif
};

namespace Auth // RFC1416
{
    enum Auth
    {
        REPLY = 2,
        NAME
    };
    enum Types
    {
        AUTHNULL, // Can't have enum values named NULL :/
        KERBEROS_V4,
        KERBEROS_V5,
        SPX,
        SRA = 6,
        LOKI = 10
    };
    enum Modifiers
    {
        AUTH_WHO_MASK = 1,
        AUTH_CLIENT_TO_SERVER = 0,
        AUTH_SERVER_TO_CLIENT = 1,
        AUTH_HOW_MASK = 2,
        AUTH_HOW_ONE_WAY = 0,
        AUTH_HOW_MUTUAL = 2
    };
    enum SRA
    {
        SRA_KEY = 0,
        SRA_USER = 1,
        SRA_CONTINUE = 2,
        SRA_PASSWORD = 3,
        SRA_ACCEPT = 4,
        SRA_REJECT = 5
    };

#ifdef QTTELNET_DEBUG
    QString authStr(int op)
    {
        QString str;
        switch (op) {
        case REPLY:
            str = "REPLY";
            break;
        case NAME:
            str = "NAME";
            break;
        default:
            str = QString("Unknown auth (%1)").arg(op);
        }
        return str;
    }
    QString typeStr(int op)
    {
        QString str;
        switch (op) {
        case AUTHNULL:
            str = "NULL";
            break;
        case KERBEROS_V4:
            str = "KERBEROS_V4";
            break;
        case KERBEROS_V5:
            str = "KERBEROS_V5";
            break;
        case SPX:
            str = "SPX";
            break;
        case SRA:
            str = "SRA";
            break;
        case LOKI:
            str = "LOKI";
            break;
        default:
            str = QString("Unknown auth type (%1)").arg(op);
        }
        return str;
    }
    QString whoStr(int op)
    {
        QString str;
        op = op & AUTH_WHO_MASK;
        switch (op) {
        case AUTH_CLIENT_TO_SERVER:
            str = "CLIENT";
            break;
        case AUTH_SERVER_TO_CLIENT:
            str = "SERVER";
            break;
        default:
            str = QString("Unknown who type (%1)").arg(op);
        }
        return str;
    }
    QString howStr(int op)
    {
        QString str;
        op = op & AUTH_HOW_MASK;
        switch (op) {
        case AUTH_HOW_ONE_WAY:
            str = "ONE-WAY";
            break;
        case AUTH_HOW_MUTUAL:
            str = "MUTUAL";
            break;
        default:
            str = QString("Unknown how type (%1)").arg(op);
        }
        return str;
    }
    QString sraStr(int op)
    {
        QString str;
        switch (op) {
        case SRA_KEY:
            str = "KEY";
            break;
        case SRA_REJECT:
            str = "REJECT";
            break;
        case SRA_ACCEPT:
            str = "ACCEPT";
            break;
        case SRA_USER:
            str = "USER";
            break;
        case SRA_CONTINUE:
            str = "CONTINUE";
            break;
        case SRA_PASSWORD:
            str = "PASSWORD";
            break;
        default:
            str = QString("Unknown SRA option (%1)").arg(op);
        }
        return str;
    }
#endif
};

namespace LineMode // RFC1184
{
    const char Mode = 1;
    const char ForwardMask = 2;
    const char SLC = 3;
    enum Modes
    {
        EDIT = 1,
        TRAPSIG = 2,
        MODE_ACK = 4,
        SOFT_TAB = 8,
        LIT_ECHO = 16
    };
    enum SLCs
    {
        SLC_SYNCH = 1,
        SLC_BRK = 2,
        SLC_IP = 3,
        SLC_AO =  4,
        SLC_AYT = 5,
        SLC_EOR = 6,
        SLC_ABORT = 7,
        SLC_EOF = 8,
        SLC_SUSP = 9,
        SLC_EC = 10,
        SLC_EL = 11,
        SLC_EW = 12,
        SLC_RP = 13,
        SLC_LNEXT = 14,
        SLC_XON = 15,
        SLC_XOFF = 16,
        SLC_FORW1 = 17,
        SLC_FORW2 = 18,
        SLC_MCL = 19,
        SLC_MCR = 20,
        SLC_MCWL = 21,
        SLC_MCWR = 22,
        SLC_MCBOL = 23,
        SLC_MCEOL = 24,
        SLC_INSRT = 25,
        SLC_OVER = 26,
        SLC_ECR = 27,
        SLC_EWR = 28,
        SLC_EBOL = 29,
        SLC_EEOL = 30,
        SLC_DEFAULT = 3,
        SLC_VALUE = 2,
        SLC_CANTCHANGE = 1,
        SLC_NOSUPPORT = 0,
        SLC_LEVELBITS = 3,
        SLC_ACK = 128,
        SLC_FLUSHIN = 64,
        SLC_FLUSHOUT = 32
    };
};

class QtTelnetAuthNull : public QtTelnetAuth
{
public:
    QtTelnetAuthNull() : QtTelnetAuth(0) {}

    QByteArray authStep(const QByteArray &data);
};

QByteArray QtTelnetAuthNull::authStep(const QByteArray &data)
{
    Q_ASSERT(data[0] == Common::Authentication);

    if (data.size() < 2 || data[1] != Common::SEND)
        return QByteArray();

    char buf[8] = {(char)Common::IAC, (char)Common::SB, Common::Authentication,
                   Common::IS, Auth::AUTHNULL, 0, // CLIENT|ONE-WAY
                   (char)Common::IAC, (char)Common::SE};
    setState(AuthSuccess);
    return QByteArray(buf, sizeof(buf));
}

class QtTelnetPrivate : public QObject
{
    Q_OBJECT
public:
    QtTelnetPrivate(QtTelnet *parent);
    ~QtTelnetPrivate();

    QMap<char, bool> modes;
    QList< QPair<uchar, uchar> > osent;

    QtTelnet *q;
    QTcpSocket *socket;
    QtTelnetReceiveBuffer buffer;
    QSocketNotifier *notifier;

    QSize windowSize;

    bool connected, nocheckp;
    bool triedlogin, triedpass, firsttry;

    QMap<int, QtTelnetAuth*> auths;
    QtTelnetAuth *curauth;
    bool nullauth;

    QRegExp loginp, passp, promptp;
    QString login, pass;

    bool allowOption(int oper, int opt);
    void sendOptions();
    void sendCommand(const QByteArray &command);
    void sendCommand(const char *command, int length);
    void sendCommand(const char operation, const char option);
    void sendString(const QString &str);
    bool replyNeeded(uchar operation, uchar option);
    void setMode(uchar operation, uchar option);
    bool alreadySent(uchar operation, uchar option);
    void addSent(uchar operation, uchar option);
    void sendWindowSize();

    int  parsePlaintext(const QByteArray &data);
    int parseIAC(const QByteArray &data);
    bool isOperation(const uchar c);
    bool isCommand(const uchar c);
    QByteArray getSubOption(const QByteArray &data);
    void parseSubAuth(const QByteArray &data);
    void parseSubTT(const QByteArray &data);
    void parseSubNAWS(const QByteArray &data);
    uchar opposite(uchar operation, bool positive);

    void consume();

    void setSocket(QTcpSocket *socket);

public slots:
    void socketConnected();
    void socketConnectionClosed();
    void socketReadyRead();
    void socketError(QAbstractSocket::SocketError error);
    void socketException(int);
};

QtTelnetPrivate::QtTelnetPrivate(QtTelnet *parent)
    : q(parent), socket(0), notifier(0),
      connected(false), nocheckp(false),
      triedlogin(false), triedpass(false), firsttry(true),
      curauth(0), nullauth(true),
      loginp("ogin:"), passp("assword:\\s*$")
{

    setSocket(new QTcpSocket(this));

}

QtTelnetPrivate::~QtTelnetPrivate()
{
    delete socket;
//    delete notifier;
    delete curauth;
}

void QtTelnetPrivate::setSocket(QTcpSocket *s)
{
    if (socket) {
        q->logout();
        socket->flush();
    }
    delete socket;
    socket = s;
    connected = false;
    if (socket) {
        connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
        connect(socket, SIGNAL(disconnected()),
                this, SLOT(socketConnectionClosed()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
        //QObject::connect(socket,&QTcpSocket::error(QAbstractSocket::SocketError),[=](){

        QObject::connect(socket,
               static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
                         [=](QAbstractSocket::SocketError error){
            this->socketError(error);
            qDebug()<<"Telnet socketError"<<socket->errorString();
        });
        //static_cast<void (QAbstractSocket::*)()>(&QAbstractSocket::error)

        //connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
        //        this, SLOT(socketError(QAbstractSocket::SocketError)));
    }
}

/*
   Returns the opposite value of the one we pass in.
*/
uchar QtTelnetPrivate::opposite(uchar operation, bool positive)
{
    if (operation == Common::DO)
        return (positive ? Common::WILL : Common::WONT);
    else if (operation == Common::DONT) // Not allowed to say WILL
        return Common::WONT;
    else if (operation == Common::WILL)
        return (positive ? Common::DO : Common::DONT);
    else if (operation == Common::WONT) // Not allowed to say DO
        return Common::DONT;
    return 0;
}

void QtTelnetPrivate::consume()
{
    const QByteArray data = buffer.readAll();
//    qDebug()<<"consume"<<data;
    int currpos = 0;
    int prevpos = -1;;
    while (prevpos < currpos && currpos < data.size()) {
        prevpos = currpos;
        const uchar c = uchar(data[currpos]);
        if (c == Common::DM)
            ++currpos;
        else if (c == Common::IAC)
            currpos += parseIAC(data.mid(currpos));
        else // Assume plain text
            currpos += parsePlaintext(data.mid(currpos));
    }
    if (currpos < data.size())
        buffer.push_back(data.mid(currpos));
}

bool QtTelnetPrivate::isCommand(const uchar c)
{
    return (c == Common::DM);
}

bool QtTelnetPrivate::isOperation(const uchar c)
{
    return (c == Common::WILL || c == Common::WONT
            || c == Common::DO ||c == Common::DONT);
}

QByteArray QtTelnetPrivate::getSubOption(const QByteArray &data)
{
    Q_ASSERT(!data.isEmpty() && uchar(data[0]) == Common::IAC);

    if (data.size() < 4 || uchar(data[1]) != Common::SB)
        return QByteArray();

    for (int i = 2; i < data.size() - 1; ++i) {
        if (uchar(data[i]) == Common::IAC && uchar(data[i+1]) == Common::SE) {
            return data.mid(2, i-2);
        }
    }
    return QByteArray();
}

void QtTelnetPrivate::parseSubNAWS(const QByteArray &data)
{
    Q_UNUSED(data);
}

void QtTelnetPrivate::parseSubTT(const QByteArray &data)
{
    Q_ASSERT(!data.isEmpty() && data[0] == Common::TerminalType);

    if (data.size() < 2 || data[1] != Common::SEND)
        return;

    const char c1[4] = { (char)Common::IAC, (char)Common::SB,
                         Common::TerminalType, Common::IS};
    sendCommand(c1, sizeof(c1));
    //qDebug()<<"QtTelnetPrivate::parseSubTT UNKNOWN"<<data;
//    sendString("UNKNOWN");
    const char c2[2] = { (char)Common::IAC, (char)Common::SE };
    sendCommand(c2, sizeof(c2));
}

void QtTelnetPrivate::parseSubAuth(const QByteArray &data)
{
//    qDebug()<<"QtTelnetPrivate::parseSubAuth"<<data;
    Q_ASSERT(data[0] == Common::Authentication);

    if (!curauth && data[1] == Common::SEND) {
        int pos = 2;
        while (pos < data.size() && !curauth) {
            curauth = auths[data[pos]];
            pos += 2;

            if (curauth) {
                emit q->loginRequired();
                break;
            }
        }
        if (!curauth) {
            curauth = new QtTelnetAuthNull;
            nullauth = true;
            if (loginp.isEmpty() && passp.isEmpty()) {
                qDebug()<<"loginRequired 799";
                emit q->loginRequired();
                nocheckp = true;
            }
        }
    }
    if (curauth) {
        const QByteArray a = curauth->authStep(data);
        if (!a.isEmpty())
            sendCommand(a);

        if (curauth->state() == QtTelnetAuth::AuthFailure)
            emit q->loginFailed();
        else if (curauth->state() == QtTelnetAuth::AuthSuccess) {
            if (loginp.isEmpty() && passp.isEmpty())
                emit q->loggedIn();
            if (!nullauth)
                nocheckp = true;
        }
    }
}

/*
  returns number of bytes consumed
*/
int QtTelnetPrivate::parseIAC(const QByteArray &data)
{
//    qDebug()<<"QtTelnetPrivate::parseIAC"<<data;
    if (data.isEmpty())
        return 0;

    Q_ASSERT(uchar(data.at(0)) == Common::IAC);

    if (data.size() >= 3 && isOperation(data[1])) { // IAC, Operation, Option
        const uchar operation = data[1];
        const uchar option = data[2];
        if (operation == Common::WONT && option == Common::Logout) {
            q->close();
            return 3;
        }
        if (operation == Common::DONT && option == Common::Authentication) {
            if (loginp.isEmpty() && passp.isEmpty())
                emit q->loggedIn();
            nullauth = true;
        }
        if (replyNeeded(operation, option)) {
            bool allowed = allowOption(operation, option);
            sendCommand(opposite(operation, allowed), option);
            setMode(operation, option);
        }
        return 3;
    }
    if (data.size() >= 2 && isCommand(data[1])) { // IAC Command
        return 2;
    }

    QByteArray suboption = getSubOption(data);
    if (suboption.isEmpty())
        return 0;

    // IAC SB Operation SubOption [...] IAC SE
    switch (suboption[0]) {
    case Common::Authentication:
        parseSubAuth(suboption);
        break;
    case Common::TerminalType:
        parseSubTT(suboption);
        break;
    case Common::NAWS:
        parseSubNAWS(data);
        break;
    default:
        qWarning("QtTelnetPrivate::parseIAC: unknown suboption %d",
                 quint8(suboption.at(0)));
        break;
    }
    return suboption.size() + 4;
}

int QtTelnetPrivate::parsePlaintext(const QByteArray &data)
{
    //qDebug()<<"QtTelnetPrivate::parsePlaintext"<<data;
    int consumed = 0;
    int length = data.indexOf('\0');
    if (length == -1) {
        length = data.size();
        consumed = length;
    } else {
        consumed = length + 1; // + 1 for removing '\0'
    }

    //QString text = QString::fromLocal8Bit(data.constData(), length);
    QString text = QString::fromLatin1(data.constData(), length);
//    qDebug()<<"!nocheckp="<< !nocheckp <<";nullauth"<< nullauth
//            <<";triedlogin="<< triedlogin <<";firsttry="<< firsttry
//            <<";loginp.indexIn(text)="<< loginp.indexIn(text)
//            <<";text="<< text<<";loginp="<< loginp;


    if (!nocheckp && nullauth) {
        if (!promptp.isEmpty() && promptp.indexIn(text) != -1) {
            emit q->loggedIn();
            nocheckp = true;
        }
    }
    if (!nocheckp && nullauth) {
        if (!loginp.isEmpty() && loginp.indexIn(text) != -1) {
            if (triedlogin || firsttry) {
                //emit q->message(text);    // Display the login prompt
                //text.clear();
                //qDebug()<<"loginRequired 908";
                emit q->loginRequired();  // Get a (new) login
                firsttry = false;
            }
            if (!triedlogin && !login.isEmpty()) {
                //qDebug()<<"QtTelnetPrivate::parsePlaintext login:"<<login;
                //q->sendData(login.append("\r\n"));
                sendString(login.append("\r\n"));
                triedlogin = true;
            }
        }
        if (!passp.isEmpty() && passp.indexIn(text) != -1) {
            if (triedpass || firsttry) {
                //emit q->message(text);    // Display the password prompt
                //text.clear();
                emit q->loginRequired();  // Get a (new) pass
                firsttry = false;
            }
            if (!triedpass && !pass.isEmpty()) {
                //qDebug()<<"QtTelnetPrivate::parsePlaintext pass:"<<pass;
                //q->sendData(pass.append("\r\n"));
                sendString(pass.append("\r\n"));
                triedpass = true;
                // We don't have to store the password anymore
                pass.fill(' ');
                pass.resize(0);

            }
        }
    }

    if (!text.isEmpty())
        emit q->message(data);
    return consumed;
}

bool QtTelnetPrivate::replyNeeded(uchar operation, uchar option)
{
    if (operation == Common::DO || operation == Common::DONT) {
        // RFC854 requires that we don't acknowledge
        // requests to enter a mode we're already in
        if (operation == Common::DO && modes[option])
            return false;
        if (operation == Common::DONT && !modes[option])
            return false;
    }
    return true;
}

void QtTelnetPrivate::setMode(uchar operation, uchar option)
{
    if (operation != Common::DO && operation != Common::DONT)
        return;

    modes[option] = (operation == Common::DO);
    if (option == Common::NAWS && modes[Common::NAWS])
        sendWindowSize();
}

void QtTelnetPrivate::sendWindowSize()
{
    if (!modes[Common::NAWS])
        return;
    if (!q->isValidWindowSize())
        return;

    short h = qToBigEndian(windowSize.height());
    short w = qToBigEndian(windowSize.width());
    const char c[9] = { (char)Common::IAC, (char)Common::SB, Common::NAWS,
                        (w & 0x00ff), (w >> 8), (h & 0x00ff), (h >> 8),
                        (char)Common::IAC, (char)Common::SE };
    sendCommand(c, sizeof(c));
}

void QtTelnetPrivate::addSent(uchar operation, uchar option)
{
    osent.append(QPair<uchar, uchar>(operation, option));
}

bool QtTelnetPrivate::alreadySent(uchar operation, uchar option)
{
    QPair<uchar, uchar> value(operation, option);
    if (osent.contains(value)) {
        osent.removeAll(value);
        return true;
    }
    return false;
}

void QtTelnetPrivate::sendString(const QString &str)
{
    if (!connected || str.length() == 0)
        return;
    //socket->write(str.toLocal8Bit());
    socket->write(str.toLatin1());
//    qDebug()<<"QtTelnetPrivate::sendString"<<str.toLatin1();
}

void QtTelnetPrivate::sendCommand(const QByteArray &command)
{
    if (!connected || command.isEmpty())
        return;

    if (command.size() == 3) {
        const char operation = command.at(1);
        const char option = command.at(2);
        if (alreadySent(operation, option))
            return;
        addSent(operation, option);
    }
    socket->write(command);
}

void QtTelnetPrivate::sendCommand(const char operation, const char option)
{
    const char c[3] = { (char)Common::IAC, operation, option };
    sendCommand(c, 3);
}

void QtTelnetPrivate::sendCommand(const char *command, int length)
{
    QByteArray a(command, length);
    sendCommand(a);
}

bool QtTelnetPrivate::allowOption(int /*oper*/, int opt)
{
    if (opt == Common::Authentication ||
        opt == Common::SuppressGoAhead ||
        opt == Common::LineMode ||
        opt == Common::Status ||
        opt == Common::Logout ||
        opt == Common::TerminalType)
        return true;
    if (opt == Common::NAWS && q->isValidWindowSize())
        return true;
    return false;
}

void QtTelnetPrivate::sendOptions()
{
    sendCommand(Common::WILL, Common::Authentication);
    sendCommand(Common::DO, Common::SuppressGoAhead);
    sendCommand(Common::WILL, Common::LineMode);
    sendCommand(Common::DO, Common::Status);
    if (q->isValidWindowSize())
        sendCommand(Common::WILL, Common::NAWS);
}

void QtTelnetPrivate::socketConnected()
{
    qDebug()<<"QtTelnetPrivate::socketConnected";
    connected = true;
//    delete notifier;
//    notifier = new QSocketNotifier(socket->socketDescriptor(),
//                                   QSocketNotifier::Exception);
//    notifier->moveToThread(this->thread());
//    notifier->setParent(this);
//    connect(notifier, SIGNAL(activated(int)),
//            this, SLOT(socketException(int)));
    sendOptions();
    //    QObject::connect(notifier,
    //           static_cast<void (QAbstractSocket::*)(QSocketNotifier::)>(&QAbstractSocket::error),
    //                     [=](QAbstractSocket::SocketError error){
    //        this->socketError(error);
    //        qDebug()<<"socketError"<<socket->errorString();
    //    });
    //    qDebug()<<"QtTelnetPrivate::socketConnected"
    //           << this->thread()<<this->socket->thread()<<notifier->thread();
}

void QtTelnetPrivate::socketException(int)
{
     qDebug("out-of-band data received, should handle that here!");
}

void QtTelnetPrivate::socketConnectionClosed()
{
    qDebug()<<"QtTelnetPrivate::socketConnectionClosed";
//    delete notifier;
//    notifier = 0;
    connected = false;
    emit q->loggedOut();
}

void QtTelnetPrivate::socketReadyRead()
{
    buffer.append(socket->readAll());
    consume();
}

void QtTelnetPrivate::socketError(QAbstractSocket::SocketError error)
{
    emit q->connectionError(error);
}

/*!
    \enum QtTelnet::Control

    This enum specifies control messages you can send to the Telnet server
    using sendControl().

    \value GoAhead Sends the \c GO \c AHEAD control message, meaning that the
    server can continue to send data.

    \value InterruptProcess Interrupts the current running process on
    the server. This is the equivalent of pressing \key{Ctrl+C} in most
    terminal emulators.

    \value AreYouThere Sends the \c ARE \c YOU \c THERE control
    message, to check if the connection is still alive.

    \value AbortOutput Temporarily suspends the output from the server.
    The output will resume if you send this control message again.

    \value EraseCharacter Erases the last entered character.

    \value EraseLine Erases the last line.

    \value Break Sends the \c BREAK control message.

    \value EndOfFile Sends the \c END \c OF \c FILE control message.

    \value Suspend Suspends the current running process on the server.
    Equivalent to pressing \key{Ctrl+Z} in most terminal emulators.

    \value Abort Sends the \c ABORT control message.

    \sa sendControl()
*/

/*!
    Constructs a QtTelnet object.

    You must call connectToHost() before calling any of the other
    member functions.

    The \a parent is sent to the QObject constructor.

    \sa connectToHost()
*/
QtTelnet::QtTelnet(QObject *parent)
    : /*QThread*/QObject(parent), d(new QtTelnetPrivate(this))
{
//    delayTaskTimer.setParent(0);
//    taskTimer.setParent(0);
//    delayTaskTimer.moveToThread(this->thread());
//    taskTimer.moveToThread(this->thread());

    QObject::connect(this,&QtTelnet::taskStarted, &taskTimer,
                     static_cast<void (QTimer::*)()>(&QTimer::start), Qt::QueuedConnection);
    QObject::connect(this,&QtTelnet::taskRestarted, &delayTaskTimer,
                     static_cast<void (QTimer::*)(int)>(&QTimer::start), Qt::QueuedConnection);

}

/*!
    Destroys the QtTelnet object. This will also close
    the connection to the server.

    \sa logout()
*/
QtTelnet::~QtTelnet()
{
    delete d;
}

//void QtTelnet::run(void)
void QtTelnet::start()

{
//    d=new QtTelnetPrivate(this);
//    qDebug()<<"TELNET SAP SYSTEM:"<<TELNET_SAP_SYSTEM;
    qDebug()<<"TELNET SAP SYSTEM:"<<this->objectName();

    qRegisterMetaType<TerminalState>("TerminalState");
    qRegisterMetaType<ProtocolError>("ProtocolError");
    qRegisterMetaType<ProtocolInfo>("ProtocolInfo");
    qRegisterMetaType<DlgState>("DlgState");

    patternInsert(dlgLoginFailed, "\nLogon failed\\.\nPlease try again\\.\n\n\n\nlogin:*");
    patternInsert(dlgLoginFailed, "\nLogon failed\\.");
    patternInsert(dlgLoginFailed, "\nLogon failed, MS Windows passwords are case-sensitive\\.");
    patternInsert(dlgLoginFailed, "\nPlease try again\\.\n\n\n\nlogin:*");
    patternInsert(dlgLoginFailed, QRegExp::escape("\nLogon failed, MS Windows passwords are case-sensitive."
                                                "\nPlease try again.\n\n\n\nlogin:\npassword:").toLatin1());
//    patternInsert(dlgLoginFailed, "\n\nLogon terminated because of client inactivity\\.\n\n");
    patternInsert(dlgLogonTerminated, "Logon terminated because of client inactivity\\.");
    patternInsert(dlgConnectionBroken, "Connection broken");
    patternInsert(dlgSessionClosed, "Session closed\\.");

    patternInsert(dlgSapProcessing,"^\\[F8 LOff\\] Slct \\d\\s+");
    patternInsert(dlgSapProcessing,"\\s*\\[ No \\]\\s*");
    patternInsert(dlgSapProcessing,"^\\s+159800\\d*\\s+");
    patternInsert(dlgSapProcessing,"^\\s+15980\\s+");
    patternInsert(dlgSapProcessing,"^\\s+1598\\s+");
    patternInsert(dlgSapProcessing,"^\\s+159\\s+");
    patternInsert(dlgSapProcessing,"^\\s+15\\s+");
    patternInsert(dlgSapProcessing,"^\\s+0\\s+");
    patternInsert(dlgSapProcessing,"^\\s+User\\s+RU*");
    patternInsert(dlgSapNotPosting,"Posting only\\s+possible in periods\\s+\\S+ and \\S+\\s+"
                                   "in company code RU03\\s+\\[F3 Back\\]\\s+");

//    patternInsert(dlgLoginRequired, "Georgia SoftWorks Telnet Server for Windows \\S+ Ver\\. \\S+\n"
//                                                 "Registered copy, \\d+ users enabled\\.\n\n"
//                                                 "User \\d+ of \\d+\n\nlogin:*");
    patternInsert(dlgLoginRequired, "Georgia SoftWorks Telnet Server for Windows"
                                                 "[\\s+\\S+]*[\r|\n]+"
                                                 "Registered copy, \\d+ users enabled.[\r|\n]+"
                                                 "User \\d+ of \\d+[\r|\n]+login:*");
    patternInsert(dlgPassRequired, "\\w*\npassword:");
    //patternInsert(dlgLoginProccess, login);
    //patternInsert(dlgLoginRequired, "login:");
    //patternInsert(dlgLoginRequired, "login:\xFF\xFE%\xFF\xFB\x03\xFF\xFE\"\xFF\xFC\x05");
    patternInsert(dlgLoggedIn, "\\s*Emulation DEC VT-220/320/420 selected\\.\\s+"
            "Graphics Pass PC Graphics characters without changes selected\\.\\s+"
            "ANSI Color option selected\\.\\s*"
            "Initialization in progress, please wait\\.\\.\\.*");
    patternInsert(dlgLoggedIn,  "\\s+Emulation DEC VT-220/320/420 selected.\n");
    patternInsert(dlgLoggedIn, "\\s+Graphics Pass PC Graphics characters without changes selected.\n");
    patternInsert(dlgLoggedIn, "\\s+ANSI Color option selected\\.");
    patternInsert(dlgLoggedIn, "\nInitialization in progress, please wait\\.\\.\\.\n\n");
    patternInsert(dlgLoggedIn, "Status for device CON:\\s+[-]+\\s+"
            "Lines:\\s+25\\s+Columns:\\s+80\\s+Keyboard rate:\\s+31\\s+"
            "Keyboard delay:\\s+1\\s+Code page:\\s+850");
    patternInsert(dlgLoggedIn, "Status for device CON:\\s+[-]+\\s+");
    patternInsert(dlgLoggedIn, "Status for device CON:");
    patternInsert(dlgSapIntro, "Client\\s+821\\s+User\\s+\\?\\s+Password\\s+[\\*]*\\s+"
                                               "Logon Language\\s+en");
    patternInsert(dlgSapIntro,
        "Client\\s+130\\s+User\\s+\\?\\s+Password\\s+[\\*]+\\s+Logon Language\\s+en\\s+");

    patternInsert(dlgSapIntro, "\\s*First system logon\\s+Copyright\\(c\\) SAP AG\\s+2002\\.\\s+"
            "All rights reserved\\.\\s+License w/o\\s+expiration\\s+"
            "This software\\s+product, marketed\\s+by SAP AG\\s+or its distributors,\\s+"
            "includes proprietary\\s+software components\\s+of\\s+IBM Corporation\\.\\s*");

    patternInsert(dlgSapIntro,
            "Client\\s+130\\s+"
            "FC1 Client Information\\s+"
            "User\\s+\\?|(\\S{10})\\s+[_]+\\s+"
            "Password\\s+[\\*]+\\s+130\\s+"
            "Productive Client\\s+Logon Lanenage\\s+[en\\s+]*[_]+\\s+"
            "Database:\\s+DB2 v10\\.0\\.5 Fix Pack 4\\s+"
            "Server:\\s+AIX 6\\.1 64-Bit SP 117\\s+"
            "Application:\\s+SAP EHP 7 for SAP ERP 6\\.0 /  NW 7\\.40\\s+[_]+\\s+"
            "New password policy:\\s+"
            "- 10 characters minimum length \\(digits / letters\\)\\.\\s+"
            "- Password must contain 1 lowercase, 1 uppercase and\\s+"
            "1 special character \\[ \\* \\+ \\. - # \\$  & / \\) \\( \\] \\.\\s+"
            "- User is locked after 5 invalid login attempts\\.");
    patternInsert(dlgSapIntro,
            "- 10 characters minimum length \\(digits / letters\\)\\.\\s+"
            "- Password must contain 1 lowercase, 1 uppercase and\\s+"
            "1 special character \\[ \\* \\+ \\. - # \\$  & / \\) \\( \\] \\.\\s+"
            "- User is locked after 5 invalid login attempts\\.");
    patternInsert(dlgSapIntro,
            "1 special character \\[ \\* \\+ \\. - # \\$ & / \\) \\( \\] \\. "
            "- User is locked after 5 invalid login attempts\\.");
    patternInsert(dlgSapIntro, QRegExp::escape(
            "- 10 characters minimum length (digits / letters). "
            "- Password must contain 1 lowercase, 1 uppercase and "
            "1 special character [ * + . - # $ & / ) ( ] . "
            "- User is locked after 5 invalid login attempts. ").toLatin1());
    patternInsert(dlgSapIntro, QRegExp::escape(
            "1 special character [ * + . - # $ & / ) ( ] . "
            "- User is locked after 5 invalid login attempts.").toLatin1());
    patternInsert(dlgSapIntro,
            "1 special character \\[ \\* \\+ \\. - # \\$ & / \\) \\( \\] \\. "
            "- User is locked after 5 invalid login attempts\\.");
    patternInsert(dlgSapIntro,
            "Author Message Text 1SOOMADO "
            "This application server will be restarted in 5mins "
            "Please disconnect and reconnect to SAP "
            "You'll be connected to another application server");

//          "1 special character \\[ \\* \\+ \\. - # \\$ & / \\) \\( \\] \\."
//          "- User is locked after 5 invalid login attempts\\.");

    patternInsert(dlgSapLoggedIn,
            "SAP user name\\s+(\\S{10})\\s+Last system logon\\s+(\\S{10})\\s+"
            "at\\s+(\\S{10})\\s+Copyright\\(c\\) SAP AG 2002\\.\\s+"
            "All rights reserved.\\s+License w/o expiration\\s+"
            "This software product, marketed by SAP AG\\s+or its distributors, "
            "includes proprietary\\s+ software components of\\s+IBM Corporation\\.\\s+"
            "\\(c\\) Copyright IBM Corporation 1993 - 2007\\.\\s+All rights reserved\\.\\s+"
            "IBM, DB2, and DB2 Universal Database are regis\\s+"
            "trademarks of IBM Corporation\\.");

    /*
    "SAP user name RUTYABC024 Last system logon 22.06.2016 at 13:41:03 "

    "Copyright(c) SAP AG 2002. All rights reserved. "
    "License w/o expiration This software product, marketed by SAP AG or its distributors, "
    "includes proprietary software components SAP user name RUTYABC024 "


*/
    patternInsert(dlgProcessing, "^\\s+Processing \\.\\.\\.\\s+");
    patternInsert(dlgPassStars, "^[\\*]+$");
    patternInsert(dlgSapMenu,
            "\\s*\\[ F2 Clr\\]\\s+\\[F3 Back\\]\\s+\\[ F4 Nxt\\]\\s+\\[1\\.Scan Reception >\\]\\s+\\"
            "[2\\.Stock transfer >\\]\\s+\\[3\\.Prod\\. Declarati>\\]\\s+\\"
            "[4\\.Shipping\\s+>\\]\\s+\\[F8 LOff\\] Slct\\s+");
//    patternInsert(dlgSapMenu,
//            "\\s*\\[F2 Reset\\] \\[F3 Ind\\.\\] \\[F4 Cont\\.\\] \\[1\\.Scan Reception >\\] "
//            "\\[2\\.Stock transfer >\\] \\[3\\.Prod\\. Declarati>\\] "
//            "\\[4\\.Shipping >\\] \\[F8 LOff\\] Sel\\.\\s+");
    patternInsert(dlgProdDeclMenu,
            "\\s*\\[1\\.Declare HU\\s+\\.\\.\\.\\]\\s+\\[2\\.Declare Kanban\\.\\.\\]\\s+\\[F8 LOff\\] Slct\\s+");
    patternInsert(dlgDeclKanbanFail,   "\\s+\\[F2 Clr\\]\\s+\\[F3 Back\\]\\[F8 Decl\\]\\s+"
                                       "Kanban ID number\\s+\\d+\\s+(Last Kanban scan)\\s+(\\d+)\\s+(Fail)\\s*");
    patternInsert(dlgKanbanNoPrinter,"(No default printer\\s+found in user\\s+"
        "parameter for user\\s+\\S+)\\s+\\[F3 Back\\]\\s+");
    patternInsert(dlgKanbanDeclSuccess, "[\\s*\\[F2 Clr\\]]{,1}\\s+\\[F3 Back\\]\\"
        "[F8 Decl\\]\\s+Kanban ID number\\s+[(\\d+)\\s+]*Last Kanban scan\\s+(\\d+)\\s+OK\\s+");
    patternInsert(dlgDeclKanbanOld,
        "(\\s*(\\d+){10}\\s+Old\\s+(\\d+){10}\\s*)");
    patternInsert(dlgDeclKanbanMenu,
            "\\s*\\[F2 Clr\\]\\s+\\[F3 Back\\]\\[F8 Decl\\]\\s+Kanban ID number\\s+[Last Kanban scan\\s+(\\d+)\\s+]{,1}");
    //
    patternInsert(dlgDeclKanbanMenu, "^\\s+Last Kanban scan\\s+(\\d+)\\s+$");
    //Log Off --> //F8
    patternInsert(dlgWantLogoff, "\\s*Unsaved data will\\s+be lost\\.\\s+Do you want to log\\s+off\\?"
    "\\s+\\[   Yes   \\]\\s+\\[    No   \\]\\s*");
    patternInsert(dlgKanbanInLoop, "Message:\\s+E: (Try declare\\s+production\\s+later !)\\s+Press any key\\.\\.\\.*");
//    Status for device CON:
//    ----------------------
//    Lines:          25
//    Columns:        80
//    Keyboard rate:  31
//    Keyboard delay: 1

    //Error SAP
    patternInsert(dlgSapErrorNoKanban, "\\s*(Kanban \\d+\\s+not maintained\\s+in ZJK00 table)");
    patternInsert(dlgSapLoginError, "\\s+Message:\\s+E: (Name or\\s+password is\\s+incorrect)\\s+"
                                                 "\\(repeat logon\\)\\s+Press any key\\.\\.\\.");
    patternInsert(dlgSapLoginError, "\\s+Message:\\s+E: (Password\\s+logon no longer\\s+possible - too\\s+"
                                                 "many failed\\s+attempts)\\s+Press any key\\.\\.\\.");
    patternInsert(dlgSapErrorKanbanNoProdVer,
        "Message:\\s+E: (Kanban Card\\s+without prod\\s+version)\\s+Press any key\\.\\.\\.\\s+");
    patternInsert(dlgSapErrorMessage, "\\s*(Message log created on \\S+\\s+"
                                        "\\(Information Messages\\s+\\d+\\)\\s+\\[\\]\\s+"
                                        "\\(Warning Messages\\s+\\d+\\)\\s+\\[\\]\\s+"
                                        "\\(Error Messages\\s+\\d+\\)\\s+\\[\\]\\s+"
                                        "\\(Termination Messages\\s+\\d+\\)\\s+\\[\\]\\s+"
                                        "\\(Total\\s+\\d+\\)\\s+\\[\\])\\s+");
    patternInsert(dlgSapStatusInfo,"\\s+(Status informati\\s*on for output\\s+request)\\s+\\S+\\s+\\d+/\\s+\\d+:\\s+(Compl\\."
                      "\\s+\\[PRINTED\\s+SUCCESSFUL)\\s*");
    //                                                                                   Status informati                                                                on for output                                                                   request 1,509,83                                                                7/   1: Compl.                                                                  [PRINTED                                                                        SUCCESSFUL
    patternInsert(dlgSapKanbanDataLock,"\\s*(Valuation data for\\s+material (\\S+)"
                                       "\\s+is locked by the\\s+user \\S+)\\s+\\[F3 Back\\]\\s+");
//    patternInsert(dlgSapErrorKanbanNoPrinter,"No default printer\\s+found in user\\s+"
//        "parameter for user\\s+\\S+\\s+\\[F3 Back\\]\\s+");


    setLoginPattern(patternMap.value(dlgLoginRequired));
    setPasswordPattern(patternMap.value(dlgPassRequired));

    QObject::connect(this,&QtTelnet::loginRequired,[=](){login();}); //qDebug()<<"QtTelnet::loginRequired";
//    QObject::connect(this,&QtTelnet::loggedIn,[=](){emit stateChanged(dlgsocketLoggedIn);});
//    QObject::connect(this,&QtTelnet::loggedOut,[=](){emit stateChanged(dlgCloseConnection);});
    QObject::connect(d->socket,&QTcpSocket::disconnected,[=](){emit stateChanged(dlgSoketDisconnected);});

//    QObject::connect(this,&QtTelnet::loginFailed,[=](){emit stateChanged(socketLoginFailed);});
    QObject::connect(this,&QtTelnet::disconnected,[=](){currentState.trmState=trmTelnetDisconnected;}); //???
    QObject::connect(this,&QtTelnet::message,this,&QtTelnet::parseMessage);
    QObject::connect(this,&QtTelnet::stateChanged,this,&QtTelnet::goNextStep);
    QObject::connect(this,&QtTelnet::sendString,d,&QtTelnetPrivate::sendString);
    QObject::connect(d->socket,&QTcpSocket::connected,this,&QtTelnet::connected);
    QObject::connect(d->socket,&QTcpSocket::disconnected,this,&QtTelnet::disconnected);

    delayTaskTimer.setSingleShot(true);
    QObject::connect(&delayTaskTimer,&QTimer::timeout,[=](){
//        qDebug()<<"delayTaskTimer timeout";
        connectToHost();
    });
    taskTimer.setInterval(taskTimeout*1000);
    taskTimer.setSingleShot(true);
    QObject::connect(&taskTimer,&QTimer::timeout,[=](){
        //emit stateChanged(dlgKanbanTimeuot);
        qDebug()<<"taskTimer timeout";
        if (!currentState.errState.contains(errTaskTimeout)){
//            qDebug()<<"errTaskTimeout close();";
            currentState.errState.append(errTaskTimeout);
            currentState.infState.append(infLogOff);
            qDebug()<<"taskTimer.start();";
            //taskTimer.start();
            emit taskStarted();
            //QTimer::singleShot(0,&taskTimer,[this](){taskTimer.start();});
            close();
        }
        else {
            kanbanFinish(currentState.errState.last());
//            qDebug()<<"kanbanFinish(errTaskTimeout);";
        }
//        currentState.dlgTracking.append(dlgKanbanTimeuot);
        //currentState.lastMessage = "Timeout";
        //taskFinish();

//        currentState.dlgTracking.clear();
//        currentState.errState.clear();
//        currentState.infState.clear();
//        currentState.lastMessage.clear();
//        if(d->connected)
//            close();
    });
}


void QtTelnet::patternInsert(DlgState dState, const QByteArray &pattern
                             //, TerminalState tState, ProtocolInfo pInfo, ProtocolError pError
                             )
{
//    bool escapeFlg=false;
//    patternMap.insert(dState, QRegExp(escapeFlg?(QRegExp::escape(pattern)):pattern));
    QRegExp rx(pattern);
    if (rx.isValid())
        patternMap.insert(dState, rx);
    else
        qDebug()<<"pattern not valid!"<<pattern;
/*    if (tState!=trmUnknown)
        if (!trmStateMap.contains(dState))
            trmStateMap.insert(dState, tState);
    if (pInfo!=infNoInfo)
        if (!infStateMap.values(dState).contains(pInfo))
            infStateMap.insert(dState, pInfo);
    if (pError!=errNoError)
        if (!errStateMap.values(dState).contains(pError))
            errStateMap.insert(dState, pError);
*/
}

QByteArray QtTelnet::stripCR(const QByteArray &msg)
{
   QString nmsg = QString(msg);
         nmsg.remove("\r"); //????????????
//       nmsg.remove(Telnet_Control::AUTO_WRAP_ON);
//       nmsg.remove(Telnet_Control::CLEAR_SCREEN);
       nmsg.remove(Telnet_Control::CURSOR_OFF);
       nmsg.remove(Telnet_Control::CURSOR_ON);
//       nmsg.remove(Telnet_Control::MOVE);
       nmsg.remove("\x1B(B");          // Also remove \x1B(B\x1B)0\x0F\x1B~ code
       nmsg.remove("\x1B)0");          // Also remove \x1B(B\x1B)0\x0F\x1B~ code
       nmsg.remove("\x0F\x1B~");          // Also remove \x1B(B\x1B)0\x0F\x1B~ code
       nmsg.remove(QRegExp("\033\\[[0-9;]*[A-Za-z]")); // Also remove terminal control codes
//       nmsg.remove(QRegExp("\x1B\\[\\S{0,4}"));                    // Also remove terminal control codes
       nmsg.remove(QRegExp("[\x01|\x03|\x05|\x07]"));                // Also remove \xXX codes
       nmsg.remove(QRegExp("[\xF0|\xFA|\xFD|\xFE%|\xFF]"));          // Also remove \xXX codes
       nmsg.remove("\x1B(B\x1B)0\x0F\x1B~");                         // Also remove \x1B(B\x1B)0\x0F\x1B~ code
       nmsg.remove("\x1B(B");                                        // Also remove \x1B(B\x1B)0 code
       nmsg.remove("\x1B)0\x0F\x1B~");                               // Also remove \x1B)0\x0F\x1B~ code
//       nmsg.remove("\x0F\x1B~"); // Also remove \x0F\x1B~ code
//       nmsg.remove("\x1B)0"); // Also remove \x1B)0 code
//       nmsg.remove("\x1B(B"); // Also remove \x1B(B code
       nmsg.remove("\x1B[?25h"); // Also remove \x1B[?25h code
       nmsg.remove("\x1B[?25l"); // Also remove \x1B[?25l code
       nmsg.remove(QRegExp("^\\s+$")); // Also remove spase string
//       //nmsg.remove("*"); // Also remove * code
       if (nmsg==QString("\r")||nmsg==QString("\n")||nmsg==QString("\r\n")||nmsg==QString("\n\r"))
           nmsg="";
       return nmsg.toLatin1();
   }

bool QtTelnet::parseMessage(const QByteArray &data)
{
    QString message= stripCR(data);
//    if (!message.trimmed().isEmpty())
//        if (message.trimmed().startsWith("15"))
//            qDebug()<<"QtTelnet::parseMessage"<<message.trimmed();
//        else
//            qDebug()<<"QtTelnet::parseMessage"<<message.trimmed().left(30);
    if (message.trimmed().isEmpty())
        return 1;
    QMultiMap<DlgState,QRegExp>::const_iterator p;
    for (p = patternMap.constBegin(); p != patternMap.constEnd(); ++p)
//        if (p.value().isValid())
//        if (p.value().exactMatch(message)!=-1){
        if (p.value().indexIn(message)!=-1){
            //qDebug() << " parseMessage indexIn";
            QStringList capTexts=p.value().capturedTexts();
            capTexts.removeFirst();
            //qDebug() << "parseMessage capTexts";
            emit stateChanged(p.key(), patternMap.values(p.key()).indexOf(p.value()), capTexts);
            return 0;

        }
    //qDebug() << "parseMessage no pattern";

    if (currentState.dlgTracking.last()==dlgLoginRequired || currentState.dlgTracking.last()==dlgLoginProccessing) {
        QString mes = message;
        if (QRegExp(mes.prepend("[*]*").append("[*]*")).indexIn(d->login)!=-1) {
            if (currentState.dlgTracking.last()==dlgLoginRequired)
                emit stateChanged(dlgLoginProccessing);
            return 0;
        }
    }
    //qDebug() << "parseMessage parseBuff.isEmpty()";
    if (!parseBuff.contains(message.toLatin1()))
        parseBuff.append(message.toLatin1());
    if (parseBuff.count()==2) {
//        qDebug() << "parseMessage parseBuff.count()==2";
        if (parseMessage(parseBuff.join(""))==0){
            parseBuff.clear();
            return 0;
        }
            parseBuff.clear();
    }
    qDebug() << "cannot parse:" << QString(parseBuff.join("")).replace(QRegExp("\\s+")," ");
    if (parseBuff.count()>2)
        parseBuff.clear();
    return 1;
}

void QtTelnet::goNextStep(DlgState state, int key, QStringList capTexts)
{
//    qDebug()<<state<<key;
    if (!currentState.dlgTracking.isEmpty())
        if (currentState.dlgTracking.last()==state) {
//        qDebug()<<"key"<<key;
        return;
        }
    currentState.dlgTracking.append(state);
    if (trmStateMap.contains(state))
        currentState.trmState=trmStateMap[state];
    if (errStateMap.contains(state))
        currentState.errState=errStateMap.values(state);
    if (infStateMap.contains(state))
        currentState.infState=infStateMap.values(state);
    qDebug()<<state<<key;
    if (!d->connected && state!=dlgSoketDisconnected) {
        qDebug()<<"QtTelnet: not connected"<<state<<key<<capTexts;
        return;
    }

//    if (currentState.activeTask.transaction == ZJPP07) {
        switch (state) {
        case dlgSapLoginError:
            if (!currentState.infState.contains(infTaskFinished)) {
                currentState.errState.append(errSapLoginError);
                kanbanFinish(currentState.errState.last(), capTexts.join(" "));
            }
            break;
        case dlgLoginRequired:
            currentState.trmState=trmTelnetConnecting;
            break;
        case dlgLoginProccessing:
        case dlgPassStars:
        case dlgProcessing:
        case dlgSapProcessing:
        case dlgPassRequired:
            break;
        case dlgLoggedIn:
            //currentState.trmState=trmSapConnecting;
            //TODO QTimer
            //emit sendString(QByteArray().append(currentState.activeTask.user).append("\t")
            //                .append(currentState.activeTask.pass).append("\r\n"));
            if (currentState.trmState==trmTelnetConnecting) {
//                qDebug()<<"dlgLoggedIn trmTelnetConnecting";
//                taskQueue.enqueue(currentState.activeTask);
//                currentState.trmState=trmTelnetDisconnected;
//                close();
//                kanbanFinish(errTaskTimeout); //todo change err
            }
            break;
        case dlgSapIntro:
            if (currentState.trmState==trmTelnetConnecting) {
//                qDebug()<<"SAP login"<<currentState.activeTask.user<<currentState.activeTask.pass;
                currentState.trmState=trmSapConnecting;
                emit sendString(QByteArray().append(currentState.activeTask.user).append("\t")
                                .append(currentState.activeTask.pass).append("\r\n"));
            }
            break;
        case dlgSapMenu:
            if (currentState.trmState==trmSapConnecting)
                emit sendString("3\r\n");
            if (currentState.trmState==trmTelnetConnecting) {
//                qDebug()<<"dlgSapMenu trmTelnetConnecting";
//                taskQueue.enqueue(currentState.activeTask);
                if (!currentState.infState.contains(infLogOff))
                    currentState.infState.append(infLogOff);
                emit sendString(FuncKeys::F8);
            }
            break;
        case dlgProdDeclMenu:
            if (currentState.trmState==trmSapConnecting)
                emit sendString("2\r\n");
            if (currentState.trmState==trmTelnetConnecting) {
//                qDebug()<<"dlgProdDeclMenu trmTelnetConnecting";
//                taskQueue.enqueue(currentState.activeTask);
                if (!currentState.infState.contains(infLogOff))
                    currentState.infState.append(infLogOff);
                emit sendString(FuncKeys::F8);
            }
            break;
        case dlgWantLogoff:
            emit sendString(QByteArray(Telnet_Control::DOWN_ARROW).append("\r\n"));
            break;
        case dlgKanbanDeclSuccess:
        case dlgDeclKanbanMenu:
//            qDebug()<<"case dlgKanbanDeclSuccess/dlgDeclKanbanMenu"
//                    <<currentState.trmState;
            //currentState.trmState=trmSapKanbanMenu;
        case dlgDeclKanbanFail:
//            if (currentState.trmState==trmBackToKanbanMenu)
//                emit sendString(FuncKeys::F3);
//            else
//                if (!currentState.infState.contains(infTaskFinished))
//                    kanbanFinish(state, capTexts.join(" "));

            if (currentState.trmState==trmTelnetConnecting) {
//                qDebug()<<"dlgKanbanDeclSuccess trmTelnetConnecting";
//                taskQueue.enqueue(currentState.activeTask);
                currentState.infState.append(infLogOff);
                emit sendString(FuncKeys::F3);
//                kanbanFinish(errTaskTimeout); //todo change err
            }
            if (currentState.trmState==trmSapConnecting
                    || currentState.trmState==trmSapKanbanMenu
                    ) {
                //qDebug()<<"currentState.trmState==trmSapConnecting";
                currentState.trmState=trmSapKanbanProcessing;
                //qDebug()<<"send kanbanNumber"<<currentState.activeTask.kanbanNumber;
                emit sendString(QByteArray(currentState.activeTask.kanbanNumber).append("\r\n"));
                break;
            }

            if (currentState.trmState==trmSapKanbanProcessing &&
                    !currentState.infState.contains(infTaskFinished)) {
                currentState.trmState=trmSapKanbanMenu;
                //kanbanFinish(state, capTexts.join(" "));
                if (currentState.errState.isEmpty())
                    kanbanFinish(errNoError, capTexts.join(" ").prepend("OK "));
                else
                    kanbanFinish(currentState.errState.last());

                break;
            }
            if (currentState.trmState==trmBackToKanbanMenu) {
                if(!currentState.infState.contains(infTaskFinished)) {
                    currentState.trmState=trmSapKanbanMenu;
                    currentState.infState.append(infTaskFinished);
                    if (currentState.errState.isEmpty())
                        kanbanFinish(errNoError,"OK");
                    else {
//                        qDebug()<< "kanbanFinish error" << currentState.errState;
                        kanbanFinish(currentState.errState.last());
                    }
                }
                //else qDebug()<< "trmBackToKanbanMenu infTaskFinished";
            }
            break;
        case dlgKanbanNoPrinter:
            currentState.infState.append(infNoPrinter);
            goNextStep(dlgBackToKanbanMenu, 1, capTexts);
            break;
        case dlgSapKanbanDataLock:
            currentState.infState.append(infKanbanErrorDataLock);
            goNextStep(dlgBackToKanbanMenu, 1, capTexts);
            break;
        case dlgSapErrorMessage:
            currentState.errState.append(errKanbanMessage);
            goNextStep(dlgBackToKanbanMenu, 1, capTexts);
            break;
        case dlgSapStatusInfo:
            currentState.infState.append(infSapStatusInfo);
            currentState.infState.append(infLogOff);
            close();

//            currentState.errState.append(errSapStatusInfo);
//            taskQueue.enqueue(currentState.activeTask);
//            currentState.infState.append(infTaskFinished);
//            currentState.trmState=trmReconnecting;
//            taskRunNext();
//            kanbanFinish(currentState.errState.last());
            break;
        case dlgSapErrorNoKanban:
            if (currentState.trmState==trmTelnetConnecting) {
//                qDebug()<<"dlgSapErrorNoKanban trmTelnetConnecting";
                currentState.infState.append(infLogOff);
                close();
            }
            if (currentState.trmState==trmSapKanbanProcessing) {
                currentState.errState.append(errNoKanban);
                goNextStep(dlgBackToKanbanMenu, 1, capTexts);
            }
            break;
        case dlgSapErrorKanbanNoProdVer:
            currentState.errState.append(errKanbanNoProdVer);
            goNextStep(dlgBackToKanbanMenu, 1, capTexts);
            break;
        case dlgKanbanInLoop:
            currentState.errState.append(errKanbanInLoop);
            goNextStep(dlgBackToKanbanMenu, 1, capTexts);
            break;
        case dlgBackToKanbanMenu:
            currentState.trmState=trmBackToKanbanMenu;
            if (key==1) {
                currentState.lastMessage=capTexts.join(" ");
            }
            emit sendString(FuncKeys::F3);
            break;
        case dlgConnectionBroken:
            if (currentState.trmState==trmTelnetConnecting &&
                    currentState.dlgTracking.contains(dlgLoggedIn)) {
                qDebug()<<"trmTelnetConnecting dlgConnectionBroken Server broke connection!";
                currentState.errState.append(errServerBrokeTheConnection);
                emit serverBrokeTheConnection();
            }
            currentState.trmState=trmTelnetDisconnected;
            break;
        case dlgSessionClosed:
            currentState.trmState=trmTelnetDisconnected;
//            if (!currentState.infState.contains(infTaskFinished)) {
//                qDebug()<<"dlgEndConnection";
//                if (currentState.infState.contains(infLogOff)) {
//                    currentState.trmState=trmReconnecting;
//                    taskStart();
//                }
//            }
            break;
//        case dlgKanbanDeclSuccess:
//            currentState.trmState=trmSapKanbanMenu;
//            if (!currentState.infState.contains(infTaskFinished))
//                kanbanFinish(state, capTexts.join(" "));
//            break;
        case dlgDeclKanbanOld:
            emit sendString(FuncKeys::F3);
            currentState.trmState=trmSapConnecting;
            break;
        case dlgSoketDisconnected:
            currentState.trmState=trmSocketDisconnected;
            if (currentState.infState.contains(infLogOff)
                    || !currentState.infState.contains(infTaskFinished)) {
                currentState.trmState=trmTelnetConnecting;
                currentState.infState.removeAll(infLogOff);
//                if (d->connected)
//                    qDebug()<<"dlgSoketDisconnected when d->connected==true";
                qDebug()<<"delayTaskTimer.start(3000);";
                emit taskRestarted(3000);
                //delayTaskTimer.start(3000);
                //QTimer::singleShot(0,&delayTaskTimer,[this](){delayTaskTimer.start(3000);});
            }
//            else if (!currentState.infState.contains(infTaskFinished)) {
//                currentState.infState.append(infTaskFinished);
//                kanbanFinish(errCloseConnection);
//                }
            break;
        case dlgSapNotPosting:
            currentState.errState.append(errSapNotPosting);
            kanbanFinish(currentState.errState.last());
            break;
        case dlgLoginFailed:
            qDebug()<<"Telnet login failed:"<<telnet_user<<telnet_pass;
            break;
        default:
            qDebug()<<"unknown state"<<state;
            return;
        }
//    } else {qDebug()<<"QtTelnet error currentState.activeTask.transaction != ZJPP07";}
}

//void QtTelnet::executeTransaction(SapTrans trans)
//{

//}

void QtTelnet::setTaskTimeout(int timeout)
{
    taskTimeout=timeout;
}

void QtTelnet::setDefaultUser(const QByteArray &user, const QByteArray &pass)
{
    defaultSapUser=user;
    defaultSapPass=pass;
}

void QtTelnet::taskRunNext()
{
//    qDebug()<<"QtTelnet::taskRunNext()";
    if(!taskQueue.isEmpty()) {
        if (!taskTimer.isActive()){
        //if (currentState.trmState==trmTelnetDisconnected || currentState.trmState==trmSapKanbanMenu) {
            //Task previusTask = currentState.activeTask;


            Task newTask = taskQueue.first();
//            qDebug()<< "newTask.kanbanNumber"<<newTask.kanbanNumber;
            if ((newTask.user!=currentState.activeTask.user ||
                        //currentState.trmState==trmTelnetDisconnected || (!d->connected) ||
                        !currentState.errState.isEmpty() || currentState.trmState!=trmSapKanbanMenu)
                        //&& currentState.trmState!=trmTelnetConnecting
                    ){
                currentState.activeTask=taskQueue.dequeue();
//                qDebug()<<"QtTelnet::taskRunNext reconnect"<<currentState.errState
//                       <<currentState.trmState<<currentState.activeTask.kanbanNumber;
                currentState.trmState=trmTelnetConnecting;
                if(d->connected)
//                    if (currentState.trmState!=trmSocketDisconnected)
                        close();
//                    else {
//                        qDebug()<<"currentState.trmState==trmSocketDisconnected when d->connected. d->socket->isOpen()"<<d->socket->isOpen();
//                        if (d->socket->isOpen()){
//                            qDebug()<<"d->socket->isOpen()";
//                            close();
//                        }
//                        else d->connected=false;
//                    }
                taskStart();
                connectToHost();
                return;
            }
            //currentState.infState.contains(infTaskFinished) &&
            if (currentState.trmState==trmSapKanbanMenu) {
                currentState.activeTask=taskQueue.dequeue();
//                qDebug()<<"QtTelnet::taskRunNext trmSapKanbanMenu"<<currentState.activeTask.kanbanNumber;
                taskStart();
                goNextStep(dlgDeclKanbanMenu);
                return;
            }
            qDebug()<<"QtTelnet::taskRunNext error"<<currentState.trmState;
        } // else qDebug()<<"taskTimer is Active";


//        if (currentState.trmState==trmTelnetDisconnected) {
//            this->goNextStep(dlgStartConnecting);
//        }

//        if (currentState.dlgTracking.last()==curState)
//            emit kanbanFinished(kanbanNumber, (int)LastState);


    }
    //else currentState.activeTask=emptyTask;
}

void QtTelnet::taskFinish()
{
    taskTimer.stop();
}

void QtTelnet::taskStart()
{
    currentState.dlgTracking.clear();
    currentState.errState.clear();
    currentState.infState.clear();
    currentState.lastMessage.clear();
    qDebug()<<"taskTimer.stop(); taskTimer.start();";
    emit taskStarted();
    //taskTimer.stop();
    //taskTimer.start();
//    QTimer::singleShot(0,&taskTimer,[this](){taskTimer.stop();});
//    QTimer::singleShot(0,&taskTimer,[this](){taskTimer.start();});
}


void QtTelnet::kanbanDeclare(int logKanbanId, const QByteArray &kanbanNumber, QByteArray user, QByteArray pass, int idDevice)
{
    //currentState.activeTask = (Task){ZJPP07, kanbanNumber, QByteArray(), count, user, password};
//    qDebug()<<"QtTelnet::kanbanDeclare"<<kanbanNumber;
    if(user.isEmpty() || pass.isEmpty()) //{
        return;
//        user=defaultSapUser;
//        pass=defaultSapPass;
//    }
//    if (logKanbanId==0) {
//        currentState.errState.append(errKanbanNotDefined);
//        kanbanFinish(currentState.errState.last(), kanbanNumber);
//        return;
//    }
    taskQueue.enqueue({logKanbanId, kanbanNumber, user, pass, idDevice});
//    currentState.activeTask.kanbanNumber=kanbanNumber;
//    currentState.activeTask.user=user;
//    currentState.activeTask.pass=password;
    taskRunNext();
}

void QtTelnet::kanbanFinish(ProtocolError error, const QString &capTexts)
{
//    qDebug()<<"QtTelnet::kanbanFinish"<<error;
    const QMetaObject & moInf = QtTelnet::staticMetaObject;
    QMetaEnum meInf = moInf.enumerator(moInf.indexOfEnumerator("ProtocolInfo"));
    const QMetaObject & moErr = QtTelnet::staticMetaObject;
    QMetaEnum meErr = moErr.enumerator(moErr.indexOfEnumerator("ProtocolError"));
    const QMetaObject & moDlg = QtTelnet::staticMetaObject;
    QMetaEnum meDlg = moDlg.enumerator(moDlg.indexOfEnumerator("DlgState"));

    QString message;
    for (int i=0;i<currentState.errState.count();i++)
        message.append(" ").append(meErr.valueToKey(currentState.errState.at(i)));
    for (int i=0;i<currentState.infState.count();i++)
        message.append(" ").append(meInf.valueToKey(currentState.infState.at(i)));
    int trackCnt = currentState.dlgTracking.count();
        if (trackCnt!=0) {
            if (error!=0 && currentState.errState.isEmpty())
                    message.append(" ").append(meDlg.valueToKey(currentState.dlgTracking.last()));
            if (currentState.errState.contains(errTaskTimeout) && trackCnt>1)
                if (currentState.errState.last()==errTaskTimeout)
                    message.prepend(" ").prepend(meDlg.valueToKey(currentState.dlgTracking.at(trackCnt-2)));
        }
        if (!capTexts.isEmpty())
            message.append(" ").append(capTexts);
        if (!currentState.lastMessage.isEmpty())
            message.append(" ").append(currentState.lastMessage);
        message.replace(QRegExp("\\s+")," ");
//        qDebug()<<"emit kanbanFinished"<<currentState.activeTask.kanbanNumber<<error<<message;
        taskFinish();
        emit kanbanFinished(currentState.activeTask.logKanbanId, currentState.activeTask.kanbanNumber,
                            error,currentState.activeTask.idDevice, message.trimmed());
        currentState.infState.append(infTaskFinished);
        //currentState.activeTask=emptyTask;
        taskRunNext();
//    } else {
//        qDebug()<<"QtTelnet::kanbanFinish taskTimer not is Active";
//        taskFinish();
//        taskRunNext();
//    }
}

/*!
    Calling this function will make the QtTelnet object attempt to
    connect to a Telnet server specified by the given \a host and \a
    port.

    The connected() signal is emitted if the connection
    succeeds, and the connectionError() signal is emitted if the
    connection fails. Once the connection is establishe you must call
    login().

    \sa close()
*/
void QtTelnet::connectToHost(const QString &host, quint16 port)
{
    if (d->connected)
        return;
    //qDebug()<<"QtTelnet::connectToHost"<<host;
    if (host.isEmpty())
        d->socket->connectToHost(host_addres, port);
    else
        d->socket->connectToHost(host, port);
}

/*!
    Closes the connection to a Telnet server.

    \sa connectToHost() login()
*/
void QtTelnet::close()
{
//    qDebug()<<"QtTelnet::close";
    if (!d->connected)
        return;
//    delete d->notifier;
//    d->notifier = 0;
    d->socket->disconnectFromHost();
    d->socket->close();
    d->connected = false;
}

/*!
    Sends the control message \a ctrl to the Telnet server the
    QtTelnet object is connected to.

    \sa Control sendData() sendSync()
*/
void QtTelnet::sendControl(Control ctrl)
{
    bool sendsync = false;
    char c;
    switch (ctrl) {
    case InterruptProcess: // Ctrl-C
        c = Common::IP;
        sendsync = true;
        break;
    case AbortOutput: // suspend/resume output
        c = Common::AO;
        sendsync = true;
        break;
    case Break:
        c = Common::BRK;
        break;
    case Suspend: // Ctrl-Z
        c = Common::SUSP;
        break;
    case EndOfFile:
        c = Common::CEOF;
        break;
    case Abort:
        c = Common::ABORT;
        break;
    case GoAhead:
        c = Common::GA;
        break;
    case AreYouThere:
        c = Common::AYT;
        sendsync = true;
        break;
    case EraseCharacter:
        c = Common::EC;
        break;
    case EraseLine:
        c = Common::EL;
        break;
    default:
        return;
    }
    const char command[2] = {(char)Common::IAC, c};
    d->sendCommand(command, sizeof(command));
    if (sendsync)
        sendSync();
}

/*!
    Sends the string \a data to the Telnet server. This is often a
    command the Telnet server will execute.

    \sa sendControl()
*/
void QtTelnet::sendData(const QString &data)
{
    if (!d->connected)
        return;
    QByteArray str = data.toLatin1();
    d->socket->write(str);
    qDebug() << "QtTelnet::sendData" << str;
    //d->socket->write("\r\n", 2);// Saul's Solution
}

//void QtTelnet::sendStr(const QString &str)
//{
//    if (!d->connected)
//        return;
//    QByteArray data= str.toLatin1();
//    d->socket->write(data);
//    d->socket->flush();
//}

/*!
    This function will log you out of the Telnet server.
    You cannot send any other data after sending this command.

    \sa login() sendData() sendControl()
*/
void QtTelnet::logout()
{
    d->sendCommand(Common::DO, Common::Logout);
}

/*!
    Sets the client window size to \a size.

    The width and height are given in number of characters.
    Non-visible clients should pass an invalid size (i.e. QSize()).

    \sa isValidWindowSize()
*/
void QtTelnet::setWindowSize(const QSize &size)
{
    setWindowSize(size.width(), size.height());
}

/*!
    Sets the client window size.

    The \a width and \a height are given in number of characters.

    \overload
*/
void QtTelnet::setWindowSize(int width, int height)
{
    bool wasvalid = isValidWindowSize();

    d->windowSize.setWidth(width);
    d->windowSize.setHeight(height);

    if (wasvalid && isValidWindowSize())
        d->sendWindowSize();
    else if (isValidWindowSize())
        d->sendCommand(Common::WILL, Common::NAWS);
    else if (wasvalid)
        d->sendCommand(Common::WONT, Common::NAWS);
}

/*!
    Returns the window's size. This will be an invalid size
    if the Telnet server is not using the NAWS option (RFC1073).

    \sa isValidWindowSize()
*/
QSize QtTelnet::windowSize() const
{
    return (d->modes[Common::NAWS] ? d->windowSize : QSize());
}

/*!
    Returns true if the window size is valid, i.e.
    windowSize().isValid() returns true; otherwise returns false.

    \sa setWindowSize()
*/
bool QtTelnet::isValidWindowSize() const
{
    return windowSize().isValid();
}

/*!
    Set the \a socket to be used in the communication.

    This function allows you to use your own QSocket subclass. You
    should call this function before calling connectToHost(); if you
    call it after a connection has been established the connection
    will be closed, so in all cases you will need to call
    connectToHost() after calling this function.

    \sa socket(), connectToHost(), logout()
*/
void QtTelnet::setSocket(QTcpSocket *socket)
{
    d->setSocket(socket);
}

/*!
    Returns the QTcpSocket instance used by this telnet object.

    \sa setSocket()
*/
QTcpSocket *QtTelnet::socket() const
{
    return d->socket;
}

/*!
    Sends the Telnet \c SYNC sequence, meaning that the Telnet server
    should discard any data waiting to be processed once the \c SYNC
    sequence has been received. This is sent using a TCP urgent
    notification.

    \sa sendControl()
*/
void QtTelnet::sendSync()
{
    if (!d->connected)
        return;
    d->socket->flush(); // Force the socket to send all the pending data before
                        // sending the SYNC sequence.
    //int s = d->socket->socketDescriptor();
    qDebug()<<"sendSync!!!!!!";

    //char tosend = (char)Common::DM;
    //::send(s, &tosend, 1, MSG_OOB); // Send the DATA MARK as out-of-band
    //char msg = "Q";
    //send(s, &msg, 1, MSG_OOB);

}

/*!
    Sets the expected shell prompt pattern.

    The \a pattern is used to automatically recognize when the client
    has successfully logged in. When a line is read that matches the
    \a pattern, the loggedIn() signal will be emitted.

    \sa login(), loggedIn()
*/
void QtTelnet::setPromptPattern(const QRegExp &pattern)
{
    d->promptp = pattern;
}

/*!
    \fn void QtTelnet::setPromptString(const QString &pattern)

    Sets the expected shell prompt to \a pattern.

    \overload
*/

/*!
    Sets the expected login pattern.

    The \a pattern is used to automatically recognize when the server
    asks for a username. If no username has been set, the
    loginRequired() signal will be emitted.

    \sa login()
*/
void QtTelnet::setLoginPattern(const QRegExp &pattern)
{
    d->loginp = pattern;
}

/*!
    \fn void QtTelnet::setLoginString(const QString &login)

    Sets the expected login string to \a login.

    \overload
*/

/*!
    Sets the expected password prompt pattern.

    The \a pattern is used to automatically recognize when the server
    asks for a password. If no password has been set, the loginRequired()
    signal will be emitted.

    \sa login()
*/
void QtTelnet::setPasswordPattern(const QRegExp &pattern)
{
    d->passp = pattern;
}

/*!
    \fn void QtTelnet::setPasswordString(const QString &pattern)

    Sets the expected password prompt to \a pattern.

    \overload
*/

/*!
    Sets the \a username and \a password to be used when logging in to
    the server.

    \sa setLoginPattern(), setPasswordPattern()
*/


void QtTelnet::login(const QString &username, const QString &password)
{
//    qDebug()<<"QtTelnet::login"<<username<<password;
    d->triedpass = d->triedlogin = false;

    if (username.isEmpty())
        d->login = telnet_user;
    else
        d->login = username;

    if (username.isEmpty())
        d->pass = telnet_pass;
    else
        d->pass = password;
}

/*!
    \fn void QtTelnet::loginRequired()

    This signal is emitted when the QtTelnet class sees
    that the Telnet server expects authentication and you
    have not already called login().

    As a reply to this signal you should either call
    login() or logout()

    \sa login(), logout()
*/

/*!
    \fn void QtTelnet::loginFailed()

    This signal is emitted when the login has failed.
    Do note that you might in certain cases see several
    loginRequired() signals being emitted but no
    loginFailed() signals. This is due to the Telnet
    specification not requiring the Telnet server to
    support reliable authentication methods.

    \sa login(), loginRequired()
*/

/*!
    \fn void QtTelnet::loggedOut()

    This signal is emitted when you have called logout()
    and the Telnet server has actually logged you out.

    \sa logout(), login()
*/

/*!
    \fn void QtTelnet::loggedIn()

    This signal is emitted when you have been logged in
    to the server as a result of the login() command
    being called. Do note that you might never see this
    signal even if you have been logged in, due to the
    Telnet specification not requiring Telnet servers
    to notify clients when users are logged in.

    \sa login(), setPromptPattern()
*/

/*!
    \fn void QtTelnet::connectionError(QAbstractSocket::SocketError error)

    This signal is emitted if the underlying socket
    implementation receives an error. The \a error
    argument is the same as being used in
    QSocket::connectionError()
*/

/*!
    \fn void QtTelnet::message(const QString &data)

    This signal is emitted when the QtTelnet object
    receives more \a data from the Telnet server.

    \sa sendData()
*/

#include "qttelnet.moc"

