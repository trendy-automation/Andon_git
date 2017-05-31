QT  = core core-private sql-private

QTDIR_build {
# This is only for the Qt build. Do not use externally. We mean it.
PLUGIN_TYPE = sqldrivers
load(qt_plugin)
CONFIG += install_ok
} else {

CONFIG += plugin

target.path += $$[QT_INSTALL_PLUGINS]/sqldrivers

INSTALLS += target

}
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
