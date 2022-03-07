APPNAME = CQSocketServer

include($$(MAKE_DIR)/qt_app.mk)

SOURCES += \
CQSocketServer.cpp

HEADERS += \
CQSocketServer.h

INCLUDEPATH += \
$(INC_DIR)/CSocket \
$(INC_DIR)/COS \

PRE_TARGETDEPS = \

unix:LIBS += -L$(LIB_DIR) \
-lCSocket -lCFile -lCStrUtil -lCPrintF -lCOS
