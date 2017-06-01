include(../../andon_project.pri)
#include(../shared_classes/qttelnet/qttelnet.pri)
include(../shared_classes/iostreamtheard/iostreamtheard.pri)
include(../shared_classes/dbwrapper/dbwrapper.pri)
#include(../shared_classes/websocket/websocket.pri)
#include(../shared_classes/qftp/qftp.pri)
include(../shared_classes/message_handler/message_handler.pri)
include(../shared_classes/single_apprun/single_apprun.pri)
include(../shared_classes/common_functions/common_functions.pri)
include(../shared_classes/watchdog/watchdog.pri)
include(../shared_classes/excel_report/excel_report.pri)
include(main_callbacks/main_callbacks.pri)
include(server_jsonrpc/server_jsonrpc.pri)
include(webui/webui.pri)
include(sms_service/sms_service.pri)
include(broadcastsender/bcsender.pri)
#include(qtxlsx/qtxlsx.pri)
include(sendemail/sendemail.pri)

DEFINES += DEBUG_NAME='\\"Server\\"'
DEFINES += APP_NAME='\\"SERVER\\"'

#SMTPCLIENTDESTDIR= $${DESTDIR}/../lib/SmtpClient/release
#QJSONRPCDESTDIR= $${DESTDIR}/../lib/qjsonrpc/release

#include($${ANDNPRJ_LIBPATH}/qjsonrpc/qjsonrpc.pri)
#INCLUDEPATH += $${ANDNPRJ_LIBPATH}/qjsonrpc/src \
#               $${ANDNPRJ_LIBPATH}/qjsonrpc/src/json
#LIBS += -L$${ANDNPRJ_LIBPATH}/qjsonrpc/src $${QJSONRPC_LIBS}
#QT += core network testlib

#include sms_service
#include($${ANDNPRJ_LIBPATH}/qextserialport-1.2rc/src/qextserialport.pri)
#QT       += core
#QT       -= gui

#QT      += script
#QT      += scripttools
#QT      += concurrent
QT      += gui
QT      += widgets
QT      += qml

CONFIG += console
CONFIG -= app_bundle

TARGET = andon_server

TEMPLATE = app

SOURCES += main.cpp

HEADERS += main.h


#CONFIG(debug, debug|release): CONFIG += force_debug_info
#QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
#QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
## remove possible other optimization flags
#QMAKE_CXXFLAGS_RELEASE -= -O3
#QMAKE_CXXFLAGS_RELEASE -= -O1
#QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE -= -O0
## add the desired -O0 if not present
#QMAKE_CXXFLAGS_RELEASE += -O0
#QMAKE_CFLAGS_RELEASE    = -O0
