#include "yansheePackageImportThread.h"

#include "logHelper.h"
#include "settings.h"
#include "md5.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>

const QString YansheePackageImportThread::CONFIG_KEY_FILE           { "file" };
const QString YansheePackageImportThread::CONFIG_KEY_VERSION        { "version" };
const QString YansheePackageImportThread::CONFIG_KEY_MD5            { "md5" };
const QString YansheePackageImportThread::CONFIG_KEY_FROM_VERSION_MIN   { "from_version_min" };

YansheePackageImportThread::YansheePackageImportThread(const DevPluginInterface *plugin, const QString &packagePath, const QString &deviceType, QObject *parent)
    : PackageImportThreadInterface(packagePath, deviceType, parent)
    , packagePath_(packagePath)
    , cancelRequest_(false)
    , plugin_(plugin)
    , extractProcess_(nullptr)
{}

void YansheePackageImportThread::run()
{
    emit importStart();
    upgradePackageInfo_ = {0};

    QString upgradeFolderPath = Settings::deviceUpgradeFolderAbsPath(deviceType_);
    if (packagePath_.contains("_diff_upgrade")) {  // 差分包
        QFileInfo originPkgFileInfo(packagePath_);
        upgradePackageInfo_.displayName = originPkgFileInfo.fileName();
        upgradePackageInfo_.diffPackage = true;
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

        if (cancelCheckPoint(50)) { return; }

        auto configFilePath = upgradeFolder.absoluteFilePath("config.json");
        QFile configFile(configFilePath);
        if (!configFile.open(QFile::ReadOnly)) {
            emit importFinished(false, tr("open config.json failed"));
            return;
        }
        auto jsonDocument = QJsonDocument::fromJson(configFile.readAll());
        upgradePackageInfo_.config = jsonDocument.object();
        configFile.close();
        upgradePackageInfo_.upgradeFileName = jsonDocument[CONFIG_KEY_FILE].toString();
        upgradePackageInfo_.version = jsonDocument[CONFIG_KEY_VERSION].toString();
        upgradePackageInfo_.md5 = jsonDocument[CONFIG_KEY_MD5].toString();
        auto upgradeFilePath = upgradeFolder.absoluteFilePath(upgradePackageInfo_.upgradeFileName);
        QString calculateMd5 = MD5::fileMd5(upgradeFilePath);
        LOG(INFO) << "calculateMd5:" << calculateMd5.toStdString();
        if (calculateMd5 != upgradePackageInfo_.md5) {
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("invalid md5, please check and retry!"));
            return;
        }

        if (cancelCheckPoint(80)) { return; }

        upgradePackageInfo_.description = QString(tr("upgrade version above %1 to %2").arg(jsonDocument[CONFIG_KEY_FROM_VERSION_MIN].toString()).arg(upgradePackageInfo_.version));
        QFileInfo upgradeFileInfo(upgradeFilePath);
        upgradePackageInfo_.upgradeFileSize = upgradeFileInfo.size();

        plugin_->writeUpgradePackageInfo(upgradePackageInfo_);

        emit importFinished(true, "");
    }
    else { // 全量包
        QFileInfo originPkgFileInfo(packagePath_);
        // yanshee_ota_v2.2.0.10_779f2ff51bb076e0d3dd260a9188f1cd.tar.gz
        auto pkgInfo = originPkgFileInfo.fileName().split("_");
        if (pkgInfo.count() != 4) {
            LOG(INFO) << "invalid pkg name:" << originPkgFileInfo.fileName().toStdString();
            emit importFinished(false, tr("invalid pkg name"));
            return;
        }
        QString versionInName = pkgInfo[2];
        if (versionInName.count() < 2) {
            LOG(INFO) << "invalid version:" << versionInName.toStdString();
            emit importFinished(false, tr("invalid version"));
            return;
        }
        versionInName = versionInName.mid(1);
        QString md5InName = pkgInfo[3].split(".")[0];

        QDir upgradeFolder(upgradeFolderPath);
        if (upgradeFolder.exists()) {
            upgradeFolder.removeRecursively();
        }
        if (!upgradeFolder.exists() && !upgradeFolder.mkpath(".")) {
            emit importFinished(false, tr("make upgrade dir failed"));
            return;
        }
        if (cancelCheckPoint(10)) { return; }

        QFile originPkgFile(packagePath_);
        if (!originPkgFile.open(QFile::ReadOnly)) {
            LOG(ERROR) << "open " << packagePath_.toStdString() << " failed!";
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("open origin upgrade pkg failed!"));
            return;
        }
        QDataStream is(&originPkgFile);
        QString destPkgPath = upgradeFolder.absoluteFilePath(originPkgFileInfo.fileName());
        QFile destPkgFile(destPkgPath);
        if (!destPkgFile.open(QFile::WriteOnly)) {
            LOG(ERROR) << "open " << destPkgPath.toStdString() << " failed!";
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("open dest upgrade pkg failed!"));
            return;
        }
        QDataStream os(&destPkgFile);
        qint64 pkgSize = originPkgFileInfo.size();
        qint64 copiedSize = 0;
        char *byteTemp = new char[4096];
        while (!is.atEnd()) {
            if (cancelRequest_) {
                break;
            }
            int readSize = 0;
            readSize = is.readRawData(byteTemp, 4096);
            os.writeRawData(byteTemp, readSize);
            copiedSize += readSize;
            emit importProcessing(80 * double(copiedSize) / pkgSize + 10);
        }
        delete byteTemp;
        originPkgFile.close();
        destPkgFile.close();

        if (cancelCheckPoint(90)) { return; }
        QString calculateMd5 = MD5::fileMd5(destPkgPath);
        LOG(INFO) << "calculateMd5:" << calculateMd5.toStdString();
        LOG(INFO) << "md5InName:" << md5InName.toStdString();
        if (md5InName.isEmpty() || calculateMd5 != md5InName) {
            upgradeFolder.removeRecursively();
            emit importFinished(false, tr("invalid md5, please check and retry!"));
            return;
        }
        QFileInfo upgradeFileInfo(destPkgPath);
        upgradePackageInfo_.diffPackage = false;
        upgradePackageInfo_.displayName = originPkgFileInfo.fileName();
        upgradePackageInfo_.upgradeFileName = upgradeFileInfo.fileName();
        upgradePackageInfo_.upgradeFileSize = upgradeFileInfo.size();
        upgradePackageInfo_.version = versionInName;
        upgradePackageInfo_.md5 = calculateMd5;
        upgradePackageInfo_.description = QString(tr("upgrade version %1")
                                                  .arg(upgradePackageInfo_.version));
        upgradePackageInfo_.config[CONFIG_KEY_FILE] = upgradePackageInfo_.upgradeFileName;
        upgradePackageInfo_.config[CONFIG_KEY_VERSION] = upgradePackageInfo_.version;
        upgradePackageInfo_.config[CONFIG_KEY_MD5] = upgradePackageInfo_.md5;

        plugin_->writeUpgradePackageInfo(upgradePackageInfo_);

        emit importFinished(true, "");
    }
}

void YansheePackageImportThread::stopImport()
{
    cancelRequest_ = true;
}

bool YansheePackageImportThread::cancelCheckPoint(int value)
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
