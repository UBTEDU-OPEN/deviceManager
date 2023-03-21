QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += PROTOBUF_USE_DLLS

INCLUDEPATH += \
    $${TRD}/glog/include \
    $${TRD}/protobuf/include \
    $${TRD}/QXlsx/header

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# windows
win32 {
    CONFIG(debug, debug|release) {
        TARGET  = testd
        DESTDIR = $${BIN}/$${PLATFORM}/$${BUILD_MODE}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglogd
        LIBS += -L$${TRD}/protobuf/lib/$${PLATFORM}/$${BUILD_MODE} \
                -llibprotobufd
        LIBS += -L$${TRD}/QXlsx/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lQXlsx
    } else {
        TARGET  = test
        DESTDIR = $${BIN}/$${PLATFORM}/$${BUILD_MODE}
        LIBS += -L$${TRD}/glog/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lglog
        LIBS += -L$${TRD}/protobuf/lib/$${PLATFORM}/$${BUILD_MODE} \
                -llibprotobuf
        LIBS += -L$${TRD}/QXlsx/lib/$${PLATFORM}/$${BUILD_MODE} \
                -lQXlsx
    }
}

include(../install/install.pri)
