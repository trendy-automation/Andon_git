#include(../../andon_project.pri)
include(../shared_classes/qttelnet/qttelnet.pri)
include(../shared_classes/message_handler/message_handler.pri)
include(../shared_classes/single_apprun/single_apprun.pri)
include(../shared_classes/watchdog/watchdog.pri)
include(plc_station/plc_station.pri)

DEFINES += DEBUG_NAME='\\"AUTO_DECL\\"'
DEFINES += APP_NAME='\\"AUTO_DECL\\"'

CONFIG  += console

TARGET = auto_declaration

TEMPLATE = app

SOURCES += main.cpp

HEADERS +=
