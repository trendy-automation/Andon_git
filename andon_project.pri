#CONFIG  += c++11
CONFIG  += c++14

#______________APP_______________
ANDNPRJ_VERSION = 2.2
ANDNPRJ_INCLUDEPATH = $${PWD}/inc
ANDNPRJ_LIBPATH = $${PWD}/lib
#DEFINES += SMTPCLIENTDESTDIR='\\"$${SMTPCLIENTDESTDIR}\\"'
DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += BUILD_PATH='\"$${OUT_PWD}\"'
DEFINES += APP_OPTION_WATHCDOG='\\"watchdog\\"'
DEFINES += APP_OPTION_FORCE='\\"force\\"'


DEFINES += APP_VER='\\"$${ANDNPRJ_VERSION}\\"'
#______________SERVER_______________
#_______________ upd _______________
DEFINES += UDP_SRVRNAME='\\"Andon\040server\040ver.\040$${ANDNPRJ_VERSION}\\"'  #TODO  ANDNPRJ_VERSION
DEFINES += UDP_PORT=45455
DEFINES += UDP_INTERVAL=3000

#_______________ qjsonrpc _______________
DEFINES += JSONRPC_SERVER_PORT=40001
DEFINES += JSONRPC_CLIENT_PORT=40002
DEFINES += JSONRPC_SERVER_WATCHDOG_PORT=40003
DEFINES += JSONRPC_CLIENT_WATCHDOG_PORT=40004
DEFINES += JSONRPC_SERVER_SERVICENAME='\\"serverRPC\\"'
DEFINES += JSONRPC_CLIENT_SERVICENAME='\\"clientRPC\\"'
DEFINES += JSONRPC_WATCHDOG_SERVICENAME='\\"wathcdogRPC\\"'
DEFINES += JSONRPC_REPLY_TIMEOUT=30000

#_______________ sendemail _______________
DEFINES += EMAIL_DOMEN='\\"LS\\"'
DEFINES += EMAIL_AUTH=false
DEFINES += EMAIL_HOST='\\"EUEDCEXC0041.EDC.EU.CORP\\"' #10.20.114.141
DEFINES += EMAIL_PORT=25
DEFINES += EMAIL_SSL=false
DEFINES += EMAIL_ADDRESS='\\"kalugafis-andon@faurecia.com\\"'
DEFINES += EMAIL_USER='\\"7andonka\\"'
DEFINES += EMAIL_PASS='\\"And0n2016\\"'

#_______________ dbwarpper _______________
DEFINES += DB_DATABASE_FILE='\\"ANDON_DB.FDB\\"'
DEFINES += DB_CASH_INTERVAL=20000 #keep result for 20 SECONDS
DEFINES += DB_CASH_CLAEN_INTERVAL=30000 #clean result for 10 MINUTES
DEFINES += DB_QUERIES_LIMIT=60 #limit of differient queries in cash

#_______________ webui _______________
DEFINES += WUI_PATH='\\"./webui2\\"'
DEFINES += WUI_BUILD_PATH='\\"./webui2\\"' #'\\"$$PWD/WebUI\\"'
DEFINES += WUI_PORT=8081
DEFINES += WUI_CLAEN_INTERVAL=900000 #each 15 minutes
DEFINES += WUI_UPDATE_INTERVAL=15000 #each 15 SECONDS
DEFINES += WEBCHANNEL_PORT=12346


#______________ CLIENT _______________
#ANDNPRJ_LIBS = -lkeyboardwidget
#______________ FTP _______________
DEFINES += FTP_INTERVAL=3600000

#TODO add broadcast sender/resiver
#______________ OPCUA _______________
DEFINES += OPCUA_IP='\\"localhost\\"'
DEFINES += OPCUA_PORT=43344
DEFINES += OPCUA_INTERVAL=3000

#______________ REPORTS _______________
DEFINES += DEF_REPORT_INTERVAL=24*3600*1000

DISTFILES += \
    $$PWD/lib/snap7/snap7.lib \
    $$PWD/lib/snap7/snap7.dll \
    $$PWD/.gitignore

#_______ DB_FIELDS ________________
#DEFINES += DB_FIELDS='\\"{\\"fldClassName\\":\\"CLASS_NAME\\"}\\"'
