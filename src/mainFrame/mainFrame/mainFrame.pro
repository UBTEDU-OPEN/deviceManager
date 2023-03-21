# add common infomation
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app

# add dll define
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += MAINFRAME_VERSION=$${MAINFRAME_VERSION}

# add dll include path
INCLUDEPATH += \
    $${TRD}/glog/include

INCLUDEPATH += \
    $${SRC}/utils \
    $${SRC}/mainFrame/plugin

# add source/header/form/resource file
SOURCES += \
    aboutDlg.cpp \
    aboutwidget.cpp \
    addNewClassroomWidget.cpp \
    classroomContainer.cpp \
    classroomContainerTabBar.cpp \
    closewarningdialog.cpp \
    exportwidget.cpp \
    main.cpp \
    mainFrame.cpp \
    nameconflictdialog.cpp \
    platformselectiondialog.cpp \
    pluginManager.cpp \
    renamewidget.cpp \
    restartToUpgradeDlg.cpp \
    tabcontainer.cpp \
    tabwidget.cpp \
    titlebar.cpp

HEADERS += \
    aboutDlg.h \
    aboutwidget.h \
    addNewClassroomWidget.h \
    classroomContainer.h \
    classroomContainerTabBar.h \
    closewarningdialog.h \
    exportwidget.h \
    mainFrame.h \
    nameconflictdialog.h \
    platformselectiondialog.h \
    pluginManager.h \
    renamewidget.h \
    restartToUpgradeDlg.h \
    tabcontainer.h \
    tabwidget.h \
    titlebar.h

# windows
win32 {
    CONFIG(debug, debug|release) {
        DEFINES += DEBUG

        TARGET  = uTools_hmd
        DESTDIR = $${MAINFRAME}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${UTILS} \
                -lutilsd
        LIBS += -L$${DESTDIR} \
                -lplugind
    } else {
        DEFINES += RELEASE

        TARGET  = uTools_hm
        DESTDIR = $${MAINFRAME}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${UTILS} \
                -lutils
        LIBS += -L$${DESTDIR} \
                -lplugin
    }
}

FORMS += \
    aboutDlg.ui \
    aboutwidget.ui \
    addNewClassroomWidget.ui \
    closewarningdialog.ui \
    exportwidget.ui \
    nameconflictdialog.ui \
    platformselectiondialog.ui \
    renamewidget.ui \
    restartToUpgradeDlg.ui \
    tabcontainer.ui \
    tabwidget.ui \
    titlebar.ui

RESOURCES += \
    res.qrc

RC_ICONS = HM64.ico

include(../install/install.pri)

QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"



