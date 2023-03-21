#include "launcher.h"

#include "logHelper.h"

#include <QApplication>
#include <QMap>
#include <QJsonObject>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<QMap<QString,QJsonObject>>("QMap<QString,QJsonObject>");

    LogHelper::initLog(argv[0]);

    Launcher w;
    w.show();

    return a.exec();
}
