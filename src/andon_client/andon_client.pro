include(../../andon_project.pri)
include(../shared_classes/qttelnet/qttelnet.pri)
include(../shared_classes/ketcp_object/ketcp_object.pri)
include(../shared_classes/serlock_manager/serlock_manager.pri)
include(../shared_classes/qftp/qftp.pri)
include(../shared_classes/websocket/websocket.pri)
include(../shared_classes/message_handler/message_handler.pri)
include(../shared_classes/single_apprun/single_apprun.pri)
include(../shared_classes/common_functions/common_functions.pri)
include(../shared_classes/watchdog/watchdog.pri)
include(main_callbacks/main_callbacks.pri)
include(clientwebinterface/clientwebinterface.pri)
include(client_jsonrpc/client_jsonrpc.pri)
include(client_interface/client_interface.pri)
include(widgetkeyboard/widgetkeyboard.pri)
include(broadcastreceiver/broadcastreceiver.pri)
include(plc_station/plc_station.pri)
include(main_callbacks/main_callbacks.pri)
#include(opcua_client/opcua_client.pri)

DEFINES += DEBUG_NAME='\\"Client\\"'
DEFINES += APP_NAME='\\"CLIENT\\"'

#QJSONRPCDESTDIR= $${DESTDIR}/../lib/qjsonrpc/release

CONFIG  += console

#QT      += concurrent
QT      += serialport
QT      += widgets
QT      += qml

TARGET = andon_client

TEMPLATE = app

SOURCES += main.cpp

RESOURCES += clientjs.qrc


############       qextserialport.pri      #################
CONFIG(release, debug|release){
    BUILD_TYPE=release
    LIB_NAME=Qt5ExtSerialPort1.dll
}
CONFIG(debug, debug|release){
    BUILD_TYPE=debug
    LIB_NAME=Qt5ExtSerialPortd1.dll
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../lib/qextserialport-1.2rc/release/ -lQt5ExtSerialPort1
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../lib/qextserialport-1.2rc/debug/ -lQt5ExtSerialPortd1
else:unix:!macx: LIBS += -L$$OUT_PWD/../../lib/qextserialport-1.2rc/ -lQt5ExtSerialPort1

copydata.commands = $(COPY_FILE) \"$$shell_path($$clean_path($$OUT_PWD/../../lib/qextserialport-1.2rc/$$BUILD_TYPE/$$LIB_NAME))\" \"$$shell_path($$OUT_PWD/$$BUILD_TYPE)\"
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

INCLUDEPATH += $$PWD/../../lib/qextserialport-1.2rc/src
DEPENDPATH += $$PWD/../../lib/qextserialport-1.2rc/src

HEADERS += \
    main.h
