#include "devicePluginInterface.h"

#include "settings.h"
#include "config.h"
#include "commonPluginUI.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QSettings>


DevPluginInterface::DevPluginInterface()
    : translator_(nullptr)
    , lan_(Language::Unset)
{}

DevPluginUI *DevPluginInterface::createUI(const QString &filePath) const
{
    auto ui = new CommonPluginUI(this);
    if (!filePath.isEmpty()) {
        ui->load(filePath);
    }
    return ui;
}

void DevPluginInterface::load(const QString &f, DevPluginUI *ui) const
{
    ui->load(f);
}

void DevPluginInterface::save(const QString &f, DevPluginUI *ui) const
{
    ui->save(f);
}

void DevPluginInterface::setAuthor(const QString &author)
{
    author_ = author;
}

QString DevPluginInterface::author() const
{
    return author_;
}

void DevPluginInterface::setDate(const QString &date)
{
    date_ = date;
}

QString DevPluginInterface::date() const
{
    return date_;
}

void DevPluginInterface::setName(const QString &name)
{
    name_ = name;
}

QString DevPluginInterface::name() const
{
    return name_;
}

void DevPluginInterface::setVersion(const QString &ver)
{
    version_ = ver;
}

QString DevPluginInterface::version() const
{
    return version_;
}

void DevPluginInterface::setDeviceType(int32_t deviceType)
{
    deviceType_ = deviceType;
}

int32_t DevPluginInterface::deviceType() const
{
    return deviceType_;
}

bool DevPluginInterface::readUpgradePackageInfo(UpgradePackageInfo &upgradePackageInfo) const
{
    QFile infoFile(upgradePkgInfoPath());
    if (infoFile.open(QFile::ReadOnly)) {
        auto jsonDocument = QJsonDocument::fromJson(infoFile.readAll());
        infoFile.close();
        upgradePackageInfo.displayName = jsonDocument[UpgradePackageInfoKey::DisplayName].toString();
        upgradePackageInfo.upgradeFileName = jsonDocument[UpgradePackageInfoKey::UpgradeFileName].toString();
        upgradePackageInfo.upgradeFileSize = jsonDocument[UpgradePackageInfoKey::UpgradeFileSize].toDouble();
        upgradePackageInfo.version = jsonDocument[UpgradePackageInfoKey::Version].toString();
        upgradePackageInfo.md5 = jsonDocument[UpgradePackageInfoKey::Md5].toString();
        upgradePackageInfo.config = jsonDocument[UpgradePackageInfoKey::Config].toObject();
        upgradePackageInfo.description = jsonDocument[UpgradePackageInfoKey::Description].toString();
        auto jsonValue = jsonDocument[UpgradePackageInfoKey::DiffPackage];
        if(!jsonValue.isUndefined()) {
            upgradePackageInfo.diffPackage = jsonDocument[UpgradePackageInfoKey::DiffPackage].toBool();
        }
    } else {
        return false;
    }
}

bool DevPluginInterface::writeUpgradePackageInfo(const UpgradePackageInfo &upgradePackageInfo) const
{
    QFile infoFile(upgradePkgInfoPath());
    if (infoFile.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonObject jsonObject;
        jsonObject[UpgradePackageInfoKey::DisplayName] = upgradePackageInfo.displayName;
        jsonObject[UpgradePackageInfoKey::UpgradeFileName] = upgradePackageInfo.upgradeFileName;
        jsonObject[UpgradePackageInfoKey::UpgradeFileSize] = upgradePackageInfo.upgradeFileSize;
        jsonObject[UpgradePackageInfoKey::Version] = upgradePackageInfo.version;
        jsonObject[UpgradePackageInfoKey::Md5] = upgradePackageInfo.md5;
        jsonObject[UpgradePackageInfoKey::Config] = upgradePackageInfo.config;
        jsonObject[UpgradePackageInfoKey::Description] = upgradePackageInfo.description;
        jsonObject[UpgradePackageInfoKey::DiffPackage] = upgradePackageInfo.diffPackage;
        QJsonDocument jsonDocument;
        jsonDocument.setObject(jsonObject);
        infoFile.write(jsonDocument.toJson());
        infoFile.close();
    } else {
        return false;
    }
}

bool DevPluginInterface::testUpgradeRequirements(int condition) const
{
    return upgradeRequirements_ & condition;
}

bool DevPluginInterface::meetUpgradeRequirements(int conditions) const
{
    return (upgradeRequirements_ & conditions) == upgradeRequirements_;
}

int DevPluginInterface::upgradeTimeLimit() const
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.beginGroup(Settings::SECTION_DEVICE_UPGRADE);
    auto limit = settings.value(Settings::KEY_UPGRADE_TIME_LIMIT, Settings::VALUE_UPGRADE_TIME_LIMIT).toInt();
    settings.endGroup();
    return limit; // sec
}

int DevPluginInterface::parallelTransferDeviceCountLimit() const
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.beginGroup(Settings::SECTION_DEVICE_UPGRADE);
    auto limit = settings.value(Settings::KEY_PARALLEL_TRANSFER_DEVICE_COUNT_LIMIT, Settings::VALUE_PARALLEL_TRANSFER_DEVICE_COUNT_LIMIT).toInt();
    settings.endGroup();
    return limit;
}

int DevPluginInterface::transferAttemptionTimesLimit() const
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.beginGroup(Settings::SECTION_DEVICE_UPGRADE);
    auto limit = settings.value(Settings::KEY_TRANSFER_ATTEMPTION_TIMES_LIMIT, Settings::VALUE_TRANSFER_ATTEMPTION_TIMES_LIMIT).toInt();
    settings.endGroup();
    return limit;
}

int DevPluginInterface::transferContinuousAttemptionTimesLimit() const
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.beginGroup(Settings::SECTION_DEVICE_UPGRADE);
    auto limit = settings.value(Settings::KEY_TRANSFER_CONTINUOUS_ATTEMPTION_TIMES_LIMIT, Settings::VALUE_TRANSFER_CONTINUOUS_ATTEMPTION_TIMES_LIMIT).toInt();
    settings.endGroup();
    return limit;
}

QString DevPluginInterface::settingsPath() const
{
    QDir pluginDir(Settings::pluginsAbsPath());
    if (pluginDir.cd(name_) && pluginDir.cd(Config::pluginsInfo()[name_])) {
        return pluginDir.absoluteFilePath("settings.ini");
    }
    return "";
}

QString DevPluginInterface::upgradePkgInfoPath() const
{
    QDir pluginDir(Settings::deviceUpgradeFolderAbsPath(name_));
    if (pluginDir.exists()) {
        return pluginDir.absoluteFilePath("info.json");
    }
    return "";
}


int versionCmp(QString ver1, QString ver2)
{
   QStringList ver1Lst;
   if (ver1.split("_").count() == 2) {
       ver1Lst = ver1.split("_")[1].split(".");
   } else {
       ver1Lst = ver1.split(".");
   }
   QStringList ver2Lst = ver2.split(".");
   for (int i = 0; i < qMin(ver1Lst.count(), ver2Lst.count()); ++i) {
       if (ver1Lst[i].toInt() < ver2Lst[i].toInt()) {
           return -1;
       } else if (ver1Lst[i].toInt() > ver2Lst[i].toInt()) {
           return 1;
       }
   }
   if (ver1Lst.count() < ver2Lst.count()) {
       return -1;
   }
   if (ver1Lst.count() > ver2Lst.count()) {
       return 1;
   }
   return 0;
}
