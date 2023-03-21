#include "mainFrame.h"

#include "logHelper.h"
#include "settings.h"

#include <QApplication>
#include <QTranslator>
#include <QFile>

#include <iostream>

#ifdef Q_OS_WIN32
#include <windows.h>
#include <winbase.h>
#pragma comment(lib,"user32")
#endif

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("uTools_hm");
    a.setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    LogHelper::initLog(argv[0]);

    a.setStyleSheet("QToolTip{background: #444A5A;color: white; border: none;}");

#ifdef Q_OS_WIN32
    LOG(INFO) << "##### win 32 os";
    if (!::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED)) {
        LOG(ERROR) << "SetThreadExecutionState failed";
    } else {
        LOG(INFO) << "SetThreadExecutionState success";
    }
#endif

    MainFrame mf;
    mf.show();

    return a.exec();
}

