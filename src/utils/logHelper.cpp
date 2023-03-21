#include "logHelper.h"
#include "settings.h"

#include <QDir>
#include <QString>
#include <QTextCodec>

#include <QDebug>

LogHelper *LogHelper::self = nullptr;

LogHelper::LogHelper(const char* argv)
{
    google::InitGoogleLogging(argv);

    auto logPrintServerity = Settings::getLogPrintServerity();
    google::SetStderrLogging(logPrintServerity);

    QString logDirPath = Settings::logFolderAbsPath();
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    logDirPath = logDir.absolutePath() + QDir::separator();
    auto logFileServerity = Settings::getLogFileServerity();
    QTextCodec *gbk_codec = QTextCodec::codecForName("GBK");
    QByteArray gbk_code = gbk_codec->fromUnicode(logDirPath.toUtf8());
    google::SetLogDestination(logFileServerity, gbk_code.data());

    FLAGS_logbufsecs = 0;
    FLAGS_max_log_size = 50;
}

LogHelper::~LogHelper()
{
    google::ShutdownGoogleLogging();
}

void LogHelper::initLog(const char* argv)
{
    if (!self) {
        self = new LogHelper(argv);
    }
}


