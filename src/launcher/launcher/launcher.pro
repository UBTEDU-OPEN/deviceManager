QT  += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += LAUNCHER_VERSION=$${LAUNCHER_VERSION}

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    $${TRD}/glog/include

INCLUDEPATH += \
    $${SRC}/utils

SOURCES += \
    main.cpp \
    launcher.cpp \
    upgradeProcessor.cpp

HEADERS += \
    launcher.h \
    upgradeProcessor.h

FORMS += \
    launcher.ui

# windows
win32 {
    CONFIG(debug, debug|release) {
        DEFINES += DEBUG
        TARGET  = launcherd
        DESTDIR = $${LAUNCHER}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${UTILS} \
                -lutilsd
    } else {
        DEFINES += RELEASE
        TARGET  = launcher
        DESTDIR = $${LAUNCHER}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${UTILS} \
                -lutils
    }
}

include(./install/install.pri)

RESOURCES += \
    launcherRes.qrc

RC_ICONS = $${SRC}/mainFrame/mainFrame/HM64.ico

QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
