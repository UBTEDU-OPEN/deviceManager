QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += AIBOXPLUGIN_LIBRARY

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

PLUGIN_NAME = "AIBox"
PLUGIN_VERSION = "1.2.0.7"

INCLUDEPATH += \
    $${TRD}/glog/include

INCLUDEPATH += \
    ../../utils \
    ../../mainFrame/plugin

SOURCES += \
    aiboxPackageImportThread.cpp \
    aiboxPlugin.cpp \
    aiboxResetConfirmProcedure.cpp \
    aiboxUpgradeConfirmProcedure.cpp \
    aiboxshutdownprocedure.cpp

HEADERS += \
    aiboxPackageImportThread.h \
    aiboxPlugin.h \
    aiboxPluginGlobal.h \
    aiboxResetConfirmProcedure.h \
    aiboxUpgradeConfirmProcedure.h \
    aiboxUpgradeConfirmProcedure.h \
    aiboxshutdownprocedure.h

# windows
win32 {
    CONFIG(debug, debug|release) {
        TARGET  = aiboxPlugind
        DESTDIR = $${PLUGINS}/$${PLUGIN_NAME}/$${PLUGIN_VERSION}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${UTILS} \
                -lutilsd
        LIBS += -L$${MAINFRAME} \
                -lplugind
    } else {
        TARGET  = aiboxPlugin
        DESTDIR = $${PLUGINS}/$${PLUGIN_NAME}/$${PLUGIN_VERSION}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${UTILS} \
                -lutils
        LIBS += -L$${MAINFRAME} \
                -lplugin
    }
}

TRANSLATIONS = aiboxPlugin_cn.ts

win32 {
QMAKE_POST_LINK += xcopy /y /i  \"$${SRC}\plugins\aiboxPlugin\res\*.*\"           \"$${DESTDIR}\res\" && \
                   xcopy /y /i  \"$${SRC}\plugins\aiboxPlugin\settings.ini\"      \"$${DESTDIR}\"     && \
                   xcopy /y /i  \"$${SRC}\plugins\aiboxPlugin\*.qm\"              \"$${DESTDIR}\"
}

DISTFILES += \
    aiboxPlugin.json \
    settings.ini

