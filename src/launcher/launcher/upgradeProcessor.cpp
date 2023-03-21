#include "upgradeProcessor.h"

#include "settings.h"
#include "config.h"
#include "logHelper.h"
#include "md5.h"
#include "fileDirHandler.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QUrl>
#include <QFile>
#include <QProcess>
#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QStandardPaths>
#include <QApplication>

UpgradeProcessor::UpgradeProcessor(QObject *parent)
    : QObject(parent)
    , upgrade_(nullptr)
    , upgradeModulesSize_(0)
    , alreadyDownloadSize_(0)
    , extractProcess_(nullptr)
    , cancelInstall_(false)
    , installing_(false)
    , preCancel_(false)
{
    modulesToDownload_.clear();
}

UpgradeProcessor::~UpgradeProcessor()
{
    if (upgrade_) {
        upgrade_->removeReplyProcessor(this);
    } else {
        LOG(WARNING) << "upgrade_ is null";
    }
}

void UpgradeProcessor::requestModuleInfo()
{
    if (upgrade_) {
        upgrade_->requestModulesInfo("en");
    } else {
        LOG(WARNING) << "upgrade_ is null";
    }
}

void UpgradeProcessor::requestGrpUpgradableByModulesInfo(const QString &mainFrameVersion)
{
    if (upgrade_) {
        QMap<QString, QString> modulesInfo;
        modulesInfo.insert("mainFrame", mainFrameVersion);
        upgrade_->requestGroupUpgradable("en", modulesInfo);
    } else {
        LOG(WARNING) << "upgrade_ is null";
    }
}

void UpgradeProcessor::requestGrpUpgradableByGrpVersion()
{
    if (upgrade_) {
        upgrade_->requestGroupUpgradable("en", Config::groupVersion());
    } else {
        LOG(WARNING) << "upgrade_ is null";
    }
}

void UpgradeProcessor::upgradeModules(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &modules)
{
//    if (modulesToDownload_.count() != 0) {
//        LOG(WARNING) << "upgrade modules is not finished yet";
//        return;
//    }
    remoteGrpVersion_ = remoteGrpVersion;
    upgradeModulesSize_ = 0;
    alreadyDownloadSize_ = 0;
    downloadedUpgradeModulesSize_.clear();
    modulesToDownload_ = modules;
    modulesToInstall_ = modules;
    if (upgrade_) {
        for (auto module : modules) {
            int startPos = 0;
            auto moduleName = module.value(Upgrade::KEY_MODULENAME).toString();
            if (UpgradeProcessor::checkLocalUpradePackage(module, startPos, upgradeModulesSize_)) {
                auto urlStr = module.value(Upgrade::KEY_PACKAGEURL).toString();
                upgrade_->download(moduleName, urlStr, startPos);
            } else {
                modulesToDownload_.remove(moduleName);
            }
            alreadyDownloadSize_ += startPos;
        }
        if (modulesToDownload_.count() == 0) {
            emit downloadFinished();
            installModules();
        }
    } else {
        LOG(WARNING) << "upgrade_ is null";
    }
}

void UpgradeProcessor::removeUpgradeFolder()
{
    QDir upgradeFolder("../../upgrade");
    upgradeFolder.removeRecursively();
}

void UpgradeProcessor::stopDownloadModules()
{
    if (upgrade_) {
        upgrade_->stopDownload();
    }

    removeUpgradeFolder();

    emit upgradeCanceled();
}

bool UpgradeProcessor::isInstalling()
{
    return installing_;
}

void UpgradeProcessor::requestCancelInstall()
{
    LOG(INFO) << "cancelInstallRequest";
    cancelInstall_ = true;
    if(installing_) {
        emit upgradeCanceled();
    }
}

bool UpgradeProcessor::checkLocalUpradePackage(const QJsonObject &moduleObj, int &startPos, qint64 &totalSize)
{
    auto urlStr = moduleObj.value(Upgrade::KEY_PACKAGEURL).toString();
    auto destFilePath = Upgrade::downloadFileUrl2DestPath(urlStr);
    auto packageSize = moduleObj.value(Upgrade::KEY_PACKAGESIZE).toInt();
    QFile destFile(destFilePath);
    if (destFile.exists()) {
        if (destFile.size() < packageSize) {
            startPos = destFile.size();
            totalSize += packageSize;
            return true;
        } else if (destFile.size() == packageSize) {
            auto packageMd5 = moduleObj.value(Upgrade::KEY_PACKAGEMD5).toString();
            if (MD5::fileMd5(destFilePath) == packageMd5) {
                startPos = packageSize;
                return false; // no need to download
            } else {
                LOG(WARNING) << "md5 of local package is not the same as remote package";
                destFile.remove();
                startPos = 0;
                totalSize += packageSize;
                return true;
            }
        } else {
            LOG(WARNING) << "local file is bigger than remote package";
            destFile.remove();
            startPos = 0;
            totalSize += packageSize;
            return true;
        }
    } else {
        startPos = 0;
        totalSize += packageSize;
        return true;
    }
}

void UpgradeProcessor::onRequestGrpUpgradableByGrpVersionReply(int httpStatusCode,
                                                               int networkError,
                                                               const QByteArray &replyData)
{
    emit groupUpgradableQueryReply(httpStatusCode, networkError, QJsonDocument::fromJson(replyData).object());
}

void UpgradeProcessor::onDownloadProgress(const QString &moduleName, qint64 bytesReceived, qint64 /*bytesTotal*/)
{
    qint64 totalDownloaded = 0;
    downloadedUpgradeModulesSize_[moduleName] = bytesReceived;
    for (auto itr = downloadedUpgradeModulesSize_.begin(); itr != downloadedUpgradeModulesSize_.end(); ++itr) {
        totalDownloaded += itr.value();
    }
    totalDownloaded += alreadyDownloadSize_;
    int progress = totalDownloaded * 100 / upgradeModulesSize_;
    emit downloadProgress(progress);
}

void UpgradeProcessor::onDownloadFinished(const QString &moduleName)
{
    modulesToDownload_.remove(moduleName);
    if (modulesToDownload_.count() == 0) {
        emit downloadFinished();
        installModules();
    }
}

void UpgradeProcessor::installModules()
{
    installing_ = true;

    if(preCancel_) {
        return;
    }

    cancelInstall_ = false;

    int total = modulesToInstall_.count();
    int i = 0;
    QMap<QString, QString> modules2ver;
    QList<QString> installed;
    auto rmInstalled = [&installed]() {
        QDir upgradePkgDir("../../upgrade/");
        upgradePkgDir.removeRecursively();
        while (installed.count() != 0) {
            QDir d(installed.takeFirst());
            LOG(INFO) << "remove " << d.absolutePath().toStdString();
            d.removeRecursively();
        }
    };
    for (auto itr = modulesToInstall_.begin(); itr != modulesToInstall_.end(); ++itr) {
        QThread::msleep(500);
        if (cancelInstall_) {
            LOG(INFO) << "cancel install";
            rmInstalled();
            emit upgradeCanceled();
            return;
        }
        auto moduleName = itr.key();
        auto moduleObj = itr.value();
        QString installPath = "../../";
        QUrl pkgUrl(moduleObj.value(Upgrade::KEY_PACKAGEURL).toString());
        QString packagePath = "../../upgrade/" + pkgUrl.fileName();
        modules2ver.insert(moduleName, moduleObj.value(Upgrade::KEY_VERSIONNAME).toString());
        if (moduleName == "launcher") {
            installPath += "launcher";
        } else if (moduleName == "mainFrame") {
            installPath += "mainFrame";
        } else { // plugins
            installPath += "plugins/" + moduleName;
        }
        QDir installDir(installPath);
        QString extractCmd = "7za.exe";
        QStringList extractArguments;
        extractArguments.append("x");
        extractArguments.append("-o" + installDir.absolutePath());
        extractArguments.append(packagePath);
        extractArguments.append("-aoa");
        extractProcess_ = new QProcess(this);
        extractProcess_->start(extractCmd, extractArguments);
        extractProcess_->waitForFinished(-1);

        installed.append(installPath + "/" + moduleObj.value(Upgrade::KEY_VERSIONNAME).toString());

        ++i;
        emit installProgress(i * 100 / total);
    }

    if (cancelInstall_) {
        LOG(INFO) << "cancel install";
        rmInstalled();
        emit upgradeCanceled();
        return;
    }

    QDir upgradePkgDir("../../upgrade/");
    upgradePkgDir.removeRecursively();
    Config::updateModulesInfo(modules2ver);
    Config::updateGroupVersion(remoteGrpVersion_);
    cleanObsoleteVersion(modules2ver);

    if(modules2ver.contains("launcher")) {
        createNewLink(modules2ver["launcher"]);
    }

    installing_ = false;
    emit installFinished();
}

void UpgradeProcessor::cleanObsoleteVersion(const QMap<QString, QString> &upgradeModules2version)
{
    for (auto itr = upgradeModules2version.begin(); itr != upgradeModules2version.end(); ++itr) {
        auto moduleName = itr.key();
        auto moduleVersion = itr.value();
        if (moduleName == "mainFrame") {
            QDir mainFrameDir("../../mainFrame");
            auto subDirsInfo = mainFrameDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (auto subDirInfo : subDirsInfo) {
                QDir subDir(subDirInfo.absoluteFilePath());
                if (subDir.dirName() != moduleVersion) {
                    if (!subDir.removeRecursively()) {
                        LOG(ERROR) << "remove dir " << subDir.absolutePath().toStdString() << " failed";
                    }
                }
            }
        } else if (moduleName == "launcher") {
#ifdef DEBUG
            QFile launcherExe("../../launcher/" + moduleVersion + "/launcherd.exe");
#else
            QFile launcherExe("../../launcher/" + moduleVersion + "/launcher.exe");
#endif
            if (launcherExe.exists()) {
                QFileInfo fileInfo(launcherExe);
                QDir dir = fileInfo.absoluteDir();
                dir.cdUp(); dir.cdUp();
                auto linkFilePath = dir.absoluteFilePath(fileInfo.baseName() + ".lnk");
                QFile linkFile(linkFilePath);
                if (linkFile.exists()) {
                    linkFile.remove();
                }
                launcherExe.link(linkFilePath);
            } else {
                LOG(ERROR) << "cant not find launcher exe in current version";
            }
        } else {
            QDir pluginDir("../../plugins/" + moduleName);
            auto subDirsInfo = pluginDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (auto subDirInfo : subDirsInfo) {
                QDir subDir(subDirInfo.absoluteFilePath());
                if (subDir.dirName() != moduleVersion) {
                    if (!subDir.removeRecursively()) {
                        LOG(ERROR) << "remove dir " << subDir.absolutePath().toStdString() << " failed";
                    }
                }
            }
        }
    }
}

void UpgradeProcessor::replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData)
{
    switch(reqType) {
    case Upgrade::RequestGroupUpgradableByGroupVersion: {
        onRequestGrpUpgradableByGrpVersionReply(httpStatusCode, networkError, replyData);
    }
        break;
    case Upgrade::DownloadFile: {
    }
        break;
    default: {
        LOG(WARNING) << "unknown request type:" << reqType;
        break;
    }
    }
    auto jsonDocument = QJsonDocument::fromJson(replyData);
    emit onReply(QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Indented)));
}

void UpgradeProcessor::startProgress()
{
    if (!upgrade_) {
        upgrade_ = new Upgrade(this);
    }
    upgrade_->addReplyProcessor(this);
    connect(upgrade_, &Upgrade::downloadProgress, this, &UpgradeProcessor::onDownloadProgress);
    connect(upgrade_, &Upgrade::downloadFinished, this, &UpgradeProcessor::onDownloadFinished);
    connect(upgrade_, &Upgrade::downloadError, this, &UpgradeProcessor::onDownloadError);

    if (Config::loadUpgradeSelection(remoteGrpVersion_, modulesToDownload_)) {
        emit startUpgrade(remoteGrpVersion_, modulesToDownload_);
    } else {
        requestGrpUpgradableByGrpVersion();
    }
}

void UpgradeProcessor::onDownloadError()
{
//    removeUpgradeFolder();
    emit downloadError();
}

void UpgradeProcessor::createNewLink(QString newVersion)
{
    QString deskTopPath = "C:/Users/Public/Desktop";
    deskTopPath += "/uTools_HM.lnk";
    bool ret = QFile::exists(deskTopPath);
    LOG(INFO) << "deskTopPath exists " << ret;
    if(ret) {
        ret = QFile::remove(deskTopPath);
#ifdef DEBUG
        QString relativePath = QString("../%1/launcherd.exe").arg(newVersion);
#else
        QString relativePath = QString("../%1/launcher.exe").arg(newVersion);
#endif
        auto newPath = FileDirHandler::absolutePath(relativePath);
        LOG(INFO) << "UpgradeProcessor::createNewLink " << newPath.toStdString();
        ret = QFile::link(newPath,deskTopPath);
    }
}

void UpgradeProcessor::cancelClicked()
{
   preCancel_ = true;
}

void UpgradeProcessor::cancelCanceled()
{
    preCancel_ = false;
    if(installing_) {
        installModules();
    }
}
