# add common infomation
QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = lib

# add dll define
DEFINES += UTILS_LIB

# add dll include path
INCLUDEPATH += \
    $${TRD}/glog/include \
    $${TRD}/QXlsx/header \
    $${TRD}/BasicExcel


# add source/header/form/resource file
SOURCES += \
    commondialog.cpp \
    config.cpp \
    logHelper.cpp \
    cmd5.cpp \
    excelHelper.cpp \
    fileDirHandler.cpp \
    md5.cpp \
    $${TRD}/BasicExcel/BasicExcel.cpp \
    settings.cpp \
    toastdialog.cpp \
    upgrade.cpp \
    warningwidget.cpp \
    cancelUpgradeConfirmDlg.cpp \
    upgradableModulesSelector.cpp

HEADERS += utilsGlobal.h \
    cmd5.h \
    commonMacro.h \
    commondialog.h \
    config.h \
    excelHelper.h \
    fileDirHandler.h \
    logHelper.h \
    md5.h \
    $${TRD}/BasicExcel/BasicExcel.hpp \
    settings.h \
    toastdialog.h \
    ubtsign.h \
    upgrade.h \
    warningwidget.h \
    cancelUpgradeConfirmDlg.h \
    upgradableModulesSelector.h

# windows
win32 {
    CONFIG(debug, debug|release) {
        TARGET = utilsd
        DESTDIR = $${UTILS}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${TRD}/QXlsx/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lQXlsx
    } else {
        TARGET = utils
        DESTDIR = $${UTILS}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${TRD}/QXlsx/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lQXlsx
        LIBS += -L$${TRD}/ubtsign/lib/win32/ -lubtsign
    }
}

FORMS += \
    commondialog.ui \
    toastdialog.ui \
    warningwidget.ui \
    cancelUpgradeConfirmDlg.ui \
    upgradableModulesSelector.ui

TRANSLATIONS = utils.ts

include(install/install.pri)

DISTFILES += \
    configs/config.json \
    configs/settings.ini \
    settings.ini

RESOURCES += \
    utilsRes.qrc

