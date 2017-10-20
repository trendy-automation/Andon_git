#include(../../andon_project.pri)
include(../shared_classes/qttelnet/qttelnet.pri)
include(../shared_classes/message_handler/message_handler.pri)
include(../shared_classes/single_apprun/single_apprun.pri)
#include(../shared_classes/watchdog/watchdog.pri)
#include(../shared_classes/common_functions/common_functions.pri)
#include(client_jsonrpc/client_jsonrpc.pri)
include(plc_station/plc_station.pri)

DEFINES += DEBUG_NAME='\\"AUTO_DECL\\"'
DEFINES += APP_NAME='\\"AUTO_DECL\\"'
DEFINES += APP_VER='\\"1.0\\"'
DEFINES += APP_OPTION_FORCE='\\"force\\"'

CONFIG  += console

TARGET = auto_declaration

TEMPLATE = app

SOURCES += main.cpp

#DISTFILES += \
#    ../../lib/snap7/snap7.lib \
#    ../../lib/snap7/snap7.dll
