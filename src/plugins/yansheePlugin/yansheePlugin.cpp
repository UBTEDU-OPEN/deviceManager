#include "yansheePlugin.h"

#include "yansheePackageImportThread.h"
#include "yansheeUpgradeConfirmProcedure.h"
#include "yansheeResetConfirmProcedure.h"
#include "yansheeshutdownprocedure.h"

#include "commonPluginUI.h"
#include "settings.h"
#include "config.h"
#include "logHelper.h"

#include <QDir>
#include <QSettings>
#include <QApplication>
#include <QTranslator>

YansheePlugin::YansheePlugin(QObject *p)
    : QObject(p)
{
    upgradeRequirements_ = (DevPluginInterface::CmdConnect | DevPluginInterface::Charging | DevPluginInterface::LockStatus | DevPluginInterface::Version);
}

void YansheePlugin::setLanguage(Language lan)
{
    if (lan != lan_) {
        lan_ = lan;
        if (translator_) {
            qApp->removeTranslator(translator_);
            translator_->deleteLater();
            translator_ = nullptr;
        }
        auto qmPath = Settings::pluginsAbsPath() + "/Yanshee/" + Config::pluginsInfo()["Yanshee"] + "/";
        switch (lan) {
        case Cn: {
            translator_ = new QTranslator(this);
            if (translator_->load(qmPath + "yansheePlugin_cn") && qApp->installTranslator(translator_)) {
                LOG(WARNING) << "load yansheePlugin_cn succeed";
            } else {
                LOG(WARNING) << "load yansheePlugin_cn failed";
                translator_->deleteLater();
                translator_ = nullptr;
            }
        }
            break;
        default: {
            translator_ = new QTranslator(this);
            if (translator_->load("yansheePlugin_cn") && qApp->installTranslator(translator_)) {
                LOG(WARNING) << "load default language file (yansheePlugin_cn) succeed";
            } else {
                LOG(WARNING) << "load default language file (yansheePlugin_cn) failed";
                translator_->deleteLater();
                translator_ = nullptr;
            }
        }
            break;
        }
    }
}

QStringList YansheePlugin::deviceTypeSelectionImagesUrls() const
{
    QStringList imageUrls;
    imageUrls.append("ic_yanshee.png");           // Normal
    imageUrls.append("ic_yanshee.png");           // Hover
    imageUrls.append("ic_yanshee_p.png");         // Pressed
    imageUrls.append("ic_yanshee_disable.png");   // Disable
    return imageUrls;
}

QStringList YansheePlugin::deviceItemImagesUrls() const
{
    QStringList imageUrls;
    imageUrls.append("ic_device_yanshee.svg");           // Normal
    imageUrls.append("ic_device_yanshee.svg");           // Hover
    imageUrls.append("ic_device_yanshee.svg");           // Pressed
    imageUrls.append("ic_device_yanshee_disable.svg");   // Disable
    return imageUrls;
}

PackageImportThreadInterface *YansheePlugin::createPackageImportThread(const QString &packagePath, QObject *parent) const
{
    return new YansheePackageImportThread(this, packagePath, name_, parent);
}

ConfirmProcedure *YansheePlugin::createUpgradeConfirmProcedure(QWidget *parent) const
{
    return new YansheeUpgradeConfirmProcedure(parent);
}

ConfirmProcedure *YansheePlugin::createResetConfirmProcedure(QWidget *parent) const
{
    return new YansheeResetConfirmProcedure(parent);
}

ConfirmProcedure *YansheePlugin::createShutDownProcedure(QWidget *parent) const
{
    return new YansheeShutDownProcedure(parent);
}

bool YansheePlugin::exclusiveLock() const
{
    return true;
}

bool YansheePlugin::obsoleteVersion(const QString &deviceVersion) const
{
    LOG(INFO) << "device version:" << deviceVersion.toStdString();
    auto res = versionCmp(deviceVersion, "2.4.0.0");
    return (res == -1);
}

bool YansheePlugin::upgradeVersionCheck(QString deviceVersion, const UpgradePackageInfo &upgradePackageInfo, QString &upgradeConfirmNotice) const
{
    bool res = false;
    upgradeConfirmNotice.clear();
    deviceVersion = deviceVersion.trimmed();
    auto upgradePkgConfig = upgradePackageInfo.config;
    auto pkgVersion = upgradePkgConfig[YansheePackageImportThread::CONFIG_KEY_VERSION].toString();
    if (upgradePkgConfig.contains(YansheePackageImportThread::CONFIG_KEY_FROM_VERSION_MIN)) {  // 差分包
        auto fromVersion = upgradePkgConfig[YansheePackageImportThread::CONFIG_KEY_FROM_VERSION_MIN].toString();
        int cmpRes = versionCmp(deviceVersion, fromVersion);
        if (cmpRes != -1) {
            cmpRes = versionCmp(deviceVersion, pkgVersion);
            if (cmpRes == 0) {
                upgradeConfirmNotice = tr("same version as upgrade package");
                res = true;
            } else if (cmpRes == -1) {
                res = true;
            }
        }
    }
    else { // 全量包
        int cmpRes = versionCmp(deviceVersion, pkgVersion);
        if (cmpRes == 0) {
            upgradeConfirmNotice = tr("same version as upgrade package");
            res = true;
        } else if (cmpRes == -1) {
            res = true;
        }
    }
    return res;
}

int YansheePlugin::cmdExcuteEstimateTime() const
{
    return 10;
}

QString YansheePlugin::upgradeNoticeText() const
{
    return tr("upgrade may take long time(>60 minutes), suggests to execute it while free time");
}

QString YansheePlugin::upgradeSuccessText() const
{
    return tr("success to upgrade");
}

QString YansheePlugin::upgradeConditionsDescription(int conditions, const UpgradePackageInfo &upgradePackageInfo) const
{
    if ((upgradeRequirements_ & DevPluginInterface::CmdConnect) && !(conditions & DevPluginInterface::CmdConnect)) {
        return tr("disconnected");
    }
    else if ((upgradeRequirements_ & DevPluginInterface::LockStatus) && !(conditions & DevPluginInterface::LockStatus)) {
        return tr("device locked");
    }
    else if (!upgradePackageInfo.upgradeFileName.isEmpty() && (upgradeRequirements_ & DevPluginInterface::Version) && !(conditions & DevPluginInterface::Version)) {
        if (upgradePackageInfo.config.contains(YansheePackageImportThread::CONFIG_KEY_FROM_VERSION_MIN)) { // 差分包
            return tr("invalid version");
        }
        else {  // 全量包
            return tr("latest version");
        }
    }
    else if ((upgradeRequirements_ & DevPluginInterface::Charging) && !(conditions & DevPluginInterface::Charging)) {
        return tr("not charging");
    }
    else {
        return (tr("connected"));
    }
}

