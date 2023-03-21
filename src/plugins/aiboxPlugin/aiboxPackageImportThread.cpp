#include "aiboxPackageImportThread.h"

#include "logHelper.h"
#include "settings.h"
#include "md5.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QSettings>

const QString AiboxPackageImportThread::CONFIG_KEY_VERSION       { "version" };
const QString AiboxPackageImportThread::CONFIG_KEY_MD5           { "MD5" };
const QString AiboxPackageImportThread::CONFIG_KEY_ALL_MD5       { "AllMd5" };
const QString AiboxPackageImportThread::CONFIG_KEY_DELTA_MD5     { "DeltaMd5" };
const QString AiboxPackageImportThread::CONFIG_KEY_FROM_VERSION  { "From_version" };
const QString AiboxPackageImportThread::CONFIG_KEY_TO_VERSION    { "To_version" };

AiboxPackageImportThread::AiboxPackageImportThread(const DevPluginInterface *plugin, const QString &packagePath, const QString &deviceType, QObject *parent)
    : PackageImportThreadInterface(packagePath, deviceType, parent)
    , packagePath_(packagePath)
    , extractProcess_(nullptr)
    , cancelRequest_(false)
    , plugin_(plugin)
{}

void AiboxPackageImportThread::run()
{
    emit importStart();

    upgradePackageInfo_ = { 0 };
    QString fileMd5;

    QString upgradeFolderPath = Settings::deviceUpgradeFolderAbsPath(deviceType_);
    QDir upgradeFolder(upgradeFolderPath);
    if (!upgradeFolder.exists() && !upgradeFolder.mkpath(".")) {
        emit importFinished(false, tr("make upgrade dir failed"));
        return;
    }
    upgradeFolder.removeRecursively();
    if (cancelCheckPoint(10)) { return; }

    QString extractCmd = "7za.exe";
    QStringList extractArguments;
    extractArguments.append("e");
    extractArguments.append("-o" + upgradeFolder.absolutePath());
    extractArguments.append(packagePath_);
    extractProcess_ = new QProcess;
    extractProcess_->start(extractCmd, extractArguments);
    extractProcess_->waitForFinished(-1);

    QSettings configFile(upgradeFolder.filePath("Config.ini"), QSettings::IniFormat);
    QStringList keysLst = configFile.allKeys();
    QSet<QString> configKeys = keysLst.toSet();
    if (configKeys.contains("Config/AllMd5")) {    // 差分升级包/新版全量包
        auto deltaMd5 = configFile.value("Config/DeltaMd5").toString();
        if (!deltaMd5.isNull() && !deltaMd5.isEmpty()) { // 差分包
            upgradePackageInfo_.diffPackage = true;
            upgradePackageInfo_.version = configFile.value("Config/To_version").toString();
            upgradePackageInfo_.md5 = configFile.value("Config/DeltaMd5").toString();
            auto from_version = configFile.value("Config/From_version").toString();
            upgradePackageInfo_.description = QString(tr("upgrade from version %1 to %2")
                                                      .arg(from_version)
                                                      .arg(upgradePackageInfo_.version));

            upgradePackageInfo_.config[CONFIG_KEY_FROM_VERSION] = from_version;
            upgradePackageInfo_.config[CONFIG_KEY_TO_VERSION] = upgradePackageInfo_.version;
            upgradePackageInfo_.config[CONFIG_KEY_ALL_MD5] = configFile.value("Config/AllMd5").toString();
            upgradePackageInfo_.config[CONFIG_KEY_DELTA_MD5] = upgradePackageInfo_.md5;
        } else {   // 全量包
            upgradePackageInfo_.diffPackage = false;
            upgradePackageInfo_.version = configFile.value("Config/To_version").toString();
            upgradePackageInfo_.md5 = configFile.value("Config/AllMd5").toString();
            upgradePackageInfo_.description = QString(tr("upgrade version %1")
                                                      .arg(upgradePackageInfo_.version));

            upgradePackageInfo_.config[CONFIG_KEY_TO_VERSION] = upgradePackageInfo_.version;
            upgradePackageInfo_.config[CONFIG_KEY_ALL_MD5] = upgradePackageInfo_.md5;
        }

        LOG(INFO) << "version:" << upgradePackageInfo_.version.toStdString();
        LOG(INFO) << "md5InConfig:" << upgradePackageInfo_.md5.toStdString();
        if (cancelCheckPoint(50)) { return; }

        QFileInfo upgradeFileInfo;
        for (auto &fileInfo : upgradeFolder.entryInfoList(QDir::Files)) {
            if (!fileInfo.fileName().endsWith(".ini")) {
                upgradeFileInfo = fileInfo;
                break;
            }
        }
        QString upgradeFilePath = upgradeFileInfo.absoluteFilePath();
        fileMd5 = MD5::fileMd5(upgradeFilePath);
        LOG(INFO) << "fileMd5:" << fileMd5.toStdString();
        if (upgradePackageInfo_.md5.isEmpty() || fileMd5 != upgradePackageInfo_.md5) {
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("invalid md5, please check and retry!"));
            return;
        }
        if (cancelCheckPoint(80)) { return; }

        QFileInfo packageFileInfo(packagePath_);
        upgradePackageInfo_.displayName = packageFileInfo.fileName();
        upgradePackageInfo_.upgradeFileName = upgradeFileInfo.fileName();
        upgradePackageInfo_.upgradeFileSize = upgradeFileInfo.size();
    }
    else {                                       // 老版本全量包
        upgradePackageInfo_.version = configFile.value("Config/version").toString();
        upgradePackageInfo_.md5 = configFile.value("Config/MD5").toString();
        upgradePackageInfo_.description = QString(tr("upgrade version:%1").arg(upgradePackageInfo_.version));
        upgradePackageInfo_.config[CONFIG_KEY_VERSION] = upgradePackageInfo_.version;
        upgradePackageInfo_.config[CONFIG_KEY_MD5] = upgradePackageInfo_.md5;
        LOG(INFO) << "version:" << upgradePackageInfo_.version.toStdString();
        LOG(INFO) << "md5InConfig:" << upgradePackageInfo_.md5.toStdString();
        if (cancelCheckPoint(50)) { return; }

        QFileInfo upgradeFileInfo;
        for (auto &fileInfo : upgradeFolder.entryInfoList(QDir::Files)) {
            if (!fileInfo.fileName().endsWith(".ini")) {
                upgradeFileInfo = fileInfo;
                break;
            }
        }
        QString upgradeFilePath = upgradeFileInfo.absoluteFilePath();
        fileMd5 = MD5::fileMd5(upgradeFilePath);
        LOG(INFO) << "fileMd5:" << fileMd5.toStdString();
        if (upgradePackageInfo_.md5.isEmpty() || fileMd5 != upgradePackageInfo_.md5) {
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("invalid md5, please check and retry!"));
            return;
        }
        if (cancelCheckPoint(80)) { return; }

        QFile upgradeFile(upgradeFilePath);
        upgradeFilePath = upgradeFilePath.replace(upgradeFileInfo.baseName(), upgradePackageInfo_.md5);
        upgradeFileInfo = QFileInfo(upgradeFilePath);
        if (!upgradeFileInfo.exists() && !upgradeFile.rename(upgradeFilePath)) {
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("rename image failed"));
            return;
        }
        if (cancelCheckPoint(90)) { return; }

        QFileInfo packageFileInfo(packagePath_);
        configFile.setValue("Config/packageName", packageFileInfo.fileName());
        upgradeFileInfo.setFile(upgradeFilePath);
        configFile.setValue("Config/imageName", upgradeFileInfo.fileName());
        configFile.sync();

        upgradePackageInfo_.displayName = packageFileInfo.fileName();
        upgradePackageInfo_.upgradeFileName = upgradeFileInfo.fileName();
        upgradePackageInfo_.upgradeFileSize = upgradeFileInfo.size();
    }
    plugin_->writeUpgradePackageInfo(upgradePackageInfo_);

    emit importFinished(true, "");
}

void AiboxPackageImportThread::stopImport()
{
    if (extractProcess_) {
        extractProcess_->kill();
        extractProcess_->deleteLater();
        extractProcess_ = nullptr;
    }
    cancelRequest_ = true;
}

bool AiboxPackageImportThread::cancelCheckPoint(int value)
{
    if (cancelRequest_) {
        QString upgradeFolderPath = Settings::deviceUpgradeFolderAbsPath(deviceType_);
        QDir upgradeFolder(upgradeFolderPath);
        upgradeFolder.removeRecursively();
        emit importCanceled();
        return true;
    } else {
        emit importProcessing(value);
        return false;
    }
}
