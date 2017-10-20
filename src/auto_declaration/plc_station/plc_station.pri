#QT += core network
include($${ANDNPRJ_LIBPATH}/snap7/snap7.pri)

#LIBS        += -lsnap7
#LIBS += -L$${ANDNPRJ_LIBPATH}/snap7 -lsnap7

INCLUDEPATH += $$PWD

HEADERS += \
#    $$PWD/snap7.h \
#    $$PWD/customkey.h \
    $$PWD/plc_station.h

SOURCES += \
#    $$PWD/snap7.cpp \
    $$PWD/plc_station.cpp



