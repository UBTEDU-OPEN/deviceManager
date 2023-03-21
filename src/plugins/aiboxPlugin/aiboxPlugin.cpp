#include "aiboxPlugin.h"

#include "aiboxPackageImportThread.h"
#include "aiboxUpgradeConfirmProcedure.h"
#include "aiboxResetConfirmProcedure.h"
#include "aiboxshutdownprocedure.h"

#include "commonPluginUI.h"
#include "settings.h"
#include "config.h"
#include "logHelper.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QTranslator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>


AiboxPlugin::AiboxPlugin(QObject *p)
    : QObject(p)
{
    upgradeRequirements_ = (DevPluginInterface::CmdConnect | DevPluginInterface::Charging | DevPluginInterface::Version);
}

void AiboxPlugin::setLanguage(Language lan)
{
    if (lan != lan_) {
        lan_ = lan;
        if (translator_) {
            qApp->removeTranslator(translator_);
            translator_->deleteLater();
            translator_ = nullptr;
        }
        auto qmPath = Settings::pluginsAbsPath() + "/AIBox/" + Config::pluginsInfo()["AIBox"] + "/";
        switch (lan) {
        case Cn: {
            translator_ = new QTranslator(this);
            if (translator_->load(qmPath + "aiboxPlugin_cn") && qApp->installTranslator(translator_)) {
                LOG(WARNING) << "load aiboxPlugin_cn succeed";
            } else {
                LOG(WARNING) << "load aiboxPlugin_cn failed";
                translator_->deleteLater();
                translator_ = nullptr;
            }
        }
            break;
        default: {
            translator_ = new QTranslator(this);
            if (translator_->load("aiboxPlugin_cn") && qApp->installTranslator(translator_)) {
                LOG(WARNING) << "load default language file (aiboxPlugin_cn) succeed";
            } else {
                LOG(WARNING) << "load default language file (aiboxPlugin_cn) failed";
                translator_->deleteLater();
                translator_ = nullptr;
            }
        }
            break;
        }
    }
}

QStringList AiboxPlugin::deviceTypeSelectionImagesUrls() const
{
    QStringList imageUrls;
    imageUrls.append("ic_aibox.png");           // Normal
    imageUrls.append("ic_aibox.png");           // Hover
    imageUrls.append("ic_aibox_p.png");         // Pressed
    imageUrls.append("ic_aibox_disable.png");   // Disable
    return imageUrls;
}

QStringList AiboxPlugin::deviceItemImagesUrls() const
{
    QStringList imageUrls;
    imageUrls.append("ic_device_aibox.svg");           // Normal
    imageUrls.append("ic_device_aibox.svg");           // Hover
    imageUrls.append("ic_device_aibox.svg");           // Pressed
    imageUrls.append("ic_device_aibox_disable.svg");   // Disable
    return imageUrls;
}

PackageImportThreadInterface *AiboxPlugin::createPackageImportThread(const QString &packagePath, QObject *parent) const
{
    return new AiboxPackageImportThread(this, packagePath, name_, parent);
}

ConfirmProcedure *AiboxPlugin::createUpgradeConfirmProcedure(QWidget *parent) const
{
    return new AiboxUpgradeConfirmProcedure(parent);
}

ConfirmProcedure *AiboxPlugin::createResetConfirmProcedure(QWidget *parent) const
{
    return new AiboxResetConfirmProcedure(parent);
}

ConfirmProcedure *AiboxPlugin::createShutDownProcedure(QWidget *parent) const
{
    return new AiboxShutDownProcedure(parent);
}

bool AiboxPlugin::upgradeVersionCheck(QString deviceVersion, const UpgradePackageInfo &upgradePackageInfo, QString &upgradeConfirmNotice) const
{
    upgradeConfirmNotice.clear();
    deviceVersion = deviceVersion.trimmed();
    auto upgradePkgConfig = upgradePackageInfo.config;
    if (upgradePkgConfig.contains(AiboxPackageImportThread::CONFIG_KEY_VERSION)) {  // 老版本全量包
        auto pkgVersion = upgradePkgConfig[AiboxPackageImportThread::CONFIG_KEY_VERSION].toString();
        return versionCmp(deviceVersion, pkgVersion) == -1;
    }
    else if (upgradePkgConfig.contains(AiboxPackageImportThread::CONFIG_KEY_FROM_VERSION)) { // 新版本增量包
        auto fromVersion = upgradePkgConfig[AiboxPackageImportThread::CONFIG_KEY_FROM_VERSION].toString();
        return versionCmp(deviceVersion, fromVersion) == 0;
    } else if (upgradePkgConfig.contains(AiboxPackageImportThread::CONFIG_KEY_TO_VERSION)) {
        auto toVersion = upgradePkgConfig[AiboxPackageImportThread::CONFIG_KEY_TO_VERSION].toString();
        return versionCmp(deviceVersion, toVersion) == -1;
    } else {
        return false;
    }
}

int AiboxPlugin::cmdExcuteEstimateTime() const
{
    return 60; // sec
}

QString AiboxPlugin::upgradeNoticeText() const
{
    return tr("upgrade may take long time, 5G wifi / cabel recommended");
}

QString AiboxPlugin::upgradeSuccessText() const
{
    return tr("upgrade success");
}

QString AiboxPlugin::upgradeConditionsDescription(int conditions, const UpgradePackageInfo &upgradePackageInfo) const
{
    if ((upgradeRequirements_ & DevPluginInterface::CmdConnect) && !(conditions & DevPluginInterface::CmdConnect)) {
        return tr("disconnected");
    }
    else if ((upgradeRequirements_ & DevPluginInterface::LockStatus) && !(conditions & DevPluginInterface::LockStatus)) {
        return tr("device locked");
    }
    else if (!upgradePackageInfo.upgradeFileName.isEmpty() && (upgradeRequirements_ & DevPluginInterface::Version) && !(conditions & DevPluginInterface::Version)) {
        if (upgradePackageInfo.config.contains(AiboxPackageImportThread::CONFIG_KEY_VERSION)) { // 老全量包
            return tr("latest version");
        }
        else if (upgradePackageInfo.config.contains(AiboxPackageImportThread::CONFIG_KEY_FROM_VERSION)) { // 新差分包
            return tr("invalid version");
        } else if (upgradePackageInfo.config.contains(AiboxPackageImportThread::CONFIG_KEY_TO_VERSION)) { // 新全量包
            return tr("latest version");
        } else {
            return tr("version error");
        }
    }
    else if ((upgradeRequirements_ & DevPluginInterface::Charging) && !(conditions & DevPluginInterface::Charging)) {
        return tr("not charging");
    }
    else {
        return (tr("connected"));
    }
}

bool AiboxPlugin::exclusiveLock() const
{
    return false;
}

bool AiboxPlugin::obsoleteVersion(const QString &deviceVersion) const
{
    LOG(INFO) << "device version:" << deviceVersion.toStdString();
    auto res = versionCmp(deviceVersion, "1.3.0.0");
    return (res == -1);
}


