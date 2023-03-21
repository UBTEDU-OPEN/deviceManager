QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib
DEFINES += PLUGIN_LIBRARY \
    PROTOBUF_USE_DLLS

CONFIG += \
    c++14 \
    plugin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    $${TRD}/glog/include \
    $${TRD}/protobuf/include

INCLUDEPATH += \
    $${SRC}/utils

SOURCES += \
    adddeviceswidget.cpp \
    backgroundwidget.cpp \
    commonClassroom.cpp \
    devicePluginInterface.cpp \
    deviceUpgradeListItemDelegate.cpp \
    deviceUpgradeThread.cpp \
    deviceUpgradeWidget.cpp \
    devicedetail.cpp \
    devicelistview.cpp \
    devicemanagement.pb.cc \
    commonDeviceItem.cpp \
    commonPluginUI.cpp \
    communicator.cpp \
    fileCopyProgressDlg.cpp \
    fileCopyThread.cpp \
    fileSelectionWidget.cpp \
    fileShareWidget.cpp \
    fileTransferThread.cpp \
    graphicsscene.cpp \
    graphicstextitem.cpp \
    listview.cpp \
    saveUserDataWidget.cpp \
    sharedFileListItemDelegate.cpp \
    transferconnectioncheckthread.cpp


HEADERS += \
    adddeviceswidget.h \
    backgroundwidget.h \
    commonClassroom.h \
    devicePluginInterface.h \
    deviceUpgradeListItemDelegate.h \
    deviceUpgradeThread.h \
    deviceUpgradeWidget.h \
    devicedetail.h \
    devicelistview.h \
    devicemanagement.pb.h \
    commonDeviceItem.h \
    commonPluginUI.h \
    communicator.h \
    fileCopyProgressDlg.h \
    fileCopyThread.h \
    fileSelectionWidget.h \
    fileShareWidget.h \
    fileTransferThread.h \
    listview.h \
    pluginGlobal.h \
    graphicsscene.h \
    graphicstextitem.h \
    pluginGlobal.h \
    saveUserDataWidget.h \
    sharedFileListItemDelegate.h \
    transferconnectioncheckthread.h

# windows
win32 {
    CONFIG(debug, debug|release) {
        DEFINES += DEBUG
        TARGET  = plugind
        DESTDIR = $${MAINFRAME}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${TRD}/protobuf/lib/$${PLATFORM}/$${BUILD_MODE} \
                -llibprotobufd
        LIBS += -L$${UTILS} \
                -lutilsd
    } else {
        DEFINES += RELEASE
        TARGET  = plugin
        DESTDIR = $${MAINFRAME}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${TRD}/protobuf/lib/$${PLATFORM}/$${BUILD_MODE} \
                -llibprotobuf
        LIBS += -L$${UTILS} \
                -lutils
    }
}

FORMS += \
    adddeviceswidget.ui \
    backgroundwiget.ui \
    deviceUpgradeWidget.ui \
    devicedetail.ui \
    fileCopyProgressDlg.ui \
    fileSelectionWidget.ui \
    fileShareWidget.ui \
    saveUserDataWidget.ui

RESOURCES += \
    res.qrc
