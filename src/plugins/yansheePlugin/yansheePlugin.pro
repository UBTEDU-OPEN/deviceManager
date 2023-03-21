QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += YANSHEEPLUGIN_LIBRARY

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

PLUGIN_NAME = "Yanshee"
PLUGIN_VERSION = "1.2.0.8"

INCLUDEPATH += \
    $${TRD}/glog/include

INCLUDEPATH += \
    ../../utils \
    ../../mainFrame/plugin

SOURCES += \
    resetModeSelectionWidget.cpp \
    yansheePackageImportThread.cpp \
    yansheePlugin.cpp \
    yansheeResetConfirmProcedure.cpp \
    yansheeUpgradeConfirmProcedure.cpp \
    yansheeshutdownprocedure.cpp

HEADERS += \
    resetModeSelectionWidget.h \
    yansheePackageImportThread.h \
    yansheePlugin.h \
    yansheePluginGlobal.h \ \
    yansheeResetConfirmProcedure.h \
    yansheeUpgradeConfirmProcedure.h \
    yansheeshutdownprocedure.h

FORMS += \
    resetModeSelectionWidget.ui

# windows
win32 {
    CONFIG(debug, debug|release) {
        TARGET  = yansheePlugind
        DESTDIR = $${PLUGINS}/$${PLUGIN_NAME}/$${PLUGIN_VERSION}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${UTILS} \
                -lutilsd
        LIBS += -L$${MAINFRAME} \
                -lplugind
    } else {
        TARGET  = yansheePlugin
        DESTDIR = $${PLUGINS}/$${PLUGIN_NAME}/$${PLUGIN_VERSION}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${UTILS} \
                -lutils
        LIBS += -L$${MAINFRAME} \
                -lplugin
    }
}

TRANSLATIONS = yansheePlugin_cn.ts

win32 {
QMAKE_POST_LINK += xcopy /y /i  \"$${SRC}\plugins\yansheePlugin\res\*.*\"      \"$${DESTDIR}\res\" && \
                   xcopy /y /i  \"$${SRC}\plugins\yansheePlugin\settings.ini\" \"$${DESTDIR}\"     && \
                   xcopy /y /i  \"$${SRC}\plugins\yansheePlugin\*.qm\"         \"$${DESTDIR}\"
}

DISTFILES += \
    settings.ini \
    yansheePlugin.json

