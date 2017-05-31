TARGET = qsqlibase
TEMPLATE = lib
QT += sql
win32:CONFIG += dll

win32{
    FIREBIRD = 'C:\Program Files\Firebird\Firebird_2_5'
} else {
    FIREBIRD = 'C:\Program Files (x86)\Firebird\Firebird_2_5'
}

#contains(QT_ARCH, i386){
#    FIREBIRD = 'C:\Program Files (x86)\Firebird\Firebird_2_5'
#} else {
#    FIREBIRD = 'C:\Program Files\Firebird\Firebird_2_5'
#}
#Add in PATH $${FIREBIRD}\bin and $${FIREBIRD}\lib
#add user andon andon
LIBS += -L$${FIREBIRD}\lib -lfbclient_ms

INCLUDEPATH += $${FIREBIRD}\include
HEADERS += \
    $$PWD/private/qsql_ibase_p.h \
    $${FIREBIRD}/include/ib_util.h \
    $${FIREBIRD}/include/ibase.h \
    $${FIREBIRD}/include/iberror.h \
    $${FIREBIRD}/include/perf.h

SOURCES += $$PWD/qsql_ibase.cpp \
           $$PWD/main.cpp

# FIXME: ignores libfb (unix)/fbclient (win32) - but that's for the test anyway
#QMAKE_USE += ibase

OTHER_FILES += ibase.json

PLUGIN_CLASS_NAME = QIBaseDriverPlugin
include(qsqldriverbase.pri)
