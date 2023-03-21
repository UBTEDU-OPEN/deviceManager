#include "config.h"

#include "logHelper.h"
#include "fileDirHandler.h"
#include "upgrade.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QDir>

const QString Config::FOLDER_CONFIG             = "../../configs";
const QString Config::FOLDER_UPGRADE            = "../../upgrade";
const QString Config::FILE_CONFIG               = "config.json";
const QString Config::FILE_UPGRADE_SELECTION    = "upgradeSelection.json";

const QString Config::KEY_GROUP_VERSION = "groupVersion";
const QString Config::KEY_LAUNCHER      = "launcher";
const QString Config::KEY_MAINFRAME     = "mainFrame";
const QString Config::KEY_PLUGINS       = "plugins";
const QString Config::KEY_NAME          = "name";
const QString Config::KEY_VERSION       = "version";

QString Config::configAbsPath()
{
    QString configPath = FOLDER_CONFIG + "/" + FILE_CONFIG;
    configPath = FileDirHandler::absolutePath(configPath);
    return configPath;
}

QString Config::groupVersion()
{
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        auto jsonDocument = QJsonDocument::fromJson(data);
        return jsonDocument[Config::KEY_GROUP_VERSION].toString();
    } else {
        LOG(ERROR) << "open config failed";
        return "";
    }
}

void Config::updateGroupVersion(const QString &newVersion)
{
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadWrite)) {
        QByteArray data = file.readAll();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto jsonObj = jsonDocument.object();
        jsonObj[Config::KEY_GROUP_VERSION] = newVersion;
        jsonDocument.setObject(jsonObj);
        file.resize(0);
        file.write(jsonDocument.toJson());
        file.close();
    } else {
        LOG(ERROR) << "open config failed";
    }
}

QString Config::laucherInfo()
{
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        auto jsonDocument = QJsonDocument::fromJson(data);
        return jsonDocument[Config::KEY_LAUNCHER].toString();
    } else {
        LOG(ERROR) << "open config failed";
        return "";
    }
}

void Config::updateLauncherInfo(const QString &version)
{
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadWrite)) {
        QByteArray data = file.readAll();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto jsonObj = jsonDocument.object();
        jsonObj[Config::KEY_LAUNCHER] = version;
        jsonDocument.setObject(jsonObj);
        file.resize(0);
        file.write(jsonDocument.toJson());
        file.close();
    } else {
        LOG(ERROR) << "open config failed";
    }
}

QString Config::mainframeInfo()
{
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        auto jsonDocument = QJsonDocument::fromJson(data);
        return jsonDocument[Config::KEY_MAINFRAME].toString();
    } else {
        LOG(ERROR) << "open config failed";
        return "";
    }
}

void Config::updateMainframeInfo(const QString &version)
{
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadWrite)) {
        QByteArray data = file.readAll();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto jsonObj = jsonDocument.object();
        jsonObj[Config::KEY_MAINFRAME] = version;
        jsonDocument.setObject(jsonObj);
        file.resize(0);
        file.write(jsonDocument.toJson());
        file.close();
    } else {
        LOG(ERROR) << "open config failed";
    }
}

QMap<QString, QString> Config::pluginsInfo()
{
    QMap<QString, QString> plugins;
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto pluginsArray = jsonDocument[Config::KEY_PLUGINS].toArray();
        for (auto pluginValue : pluginsArray) {
            auto pluginObj = pluginValue.toObject();
            plugins.insert(pluginObj[Config::KEY_NAME].toString(), pluginObj[Config::KEY_VERSION].toString());
        }
    } else {
        LOG(ERROR) << "open config failed";
    }
    return plugins;
}

QMap<QString, QString> Config::modulesInfo()
{
    QMap<QString, QString> modules;
    QFile file(Config::configAbsPath());
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto pluginsArray = jsonDocument[Config::KEY_PLUGINS].toArray();
        for (auto pluginValue : pluginsArray) {
            auto pluginObj = pluginValue.toObject();
            modules.insert(pluginObj[Config::KEY_NAME].toString(), pluginObj[Config::KEY_VERSION].toString());
        }
        modules.insert(Config::KEY_LAUNCHER, jsonDocument[Config::KEY_LAUNCHER].toString());
        modules.insert(Config::KEY_MAINFRAME, jsonDocument[Config::KEY_MAINFRAME].toString());
    } else {
        LOG(ERROR) << "open config failed";
    }
    return modules;
}

void Config::updatePluginsInfo(const QMap<QString, QString> &newPluginsInfo)
{
    QFile file(Config::configAbsPath());
    QMap<QString, QString> pluginsInfo;
    if (file.open(QFile::ReadWrite)) {
        QByteArray data = file.readAll();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto jsonObj = jsonDocument.object();
        auto pluginsArray = jsonDocument[Config::KEY_PLUGINS].toArray();
        for (auto pluginValue : pluginsArray) {
            auto pluginObj = pluginValue.toObject();
            pluginsInfo.insert(pluginObj[Config::KEY_NAME].toString(), pluginObj[Config::KEY_VERSION].toString());
        }
        for (auto itr = newPluginsInfo.begin(); itr != newPluginsInfo.end(); ++itr) {
            pluginsInfo[itr.key()] = itr.value();
        }
        QJsonArray newPluginArray;
        for (auto itr = pluginsInfo.begin(); itr != pluginsInfo.end(); ++itr) {
            QJsonObject pluginObj;
            pluginObj[Config::KEY_NAME] = itr.key();
            pluginObj[Config::KEY_VERSION] = itr.value();
            newPluginArray.append(pluginObj);
        }
        jsonObj[Config::KEY_PLUGINS] = newPluginArray;
        jsonDocument.setObject(jsonObj);
        file.resize(0);
        file.write(jsonDocument.toJson());
        file.close();
    } else {
        LOG(ERROR) << "open config failed";
    }
}

void Config::updateModulesInfo(const QMap<QString, QString> &newModulesInfo)
{
    QFile file(Config::configAbsPath());
    QMap<QString, QString> pluginsInfo;
    if (file.open(QFile::ReadWrite)) {
        QByteArray data = file.readAll();
        auto jsonDocument = QJsonDocument::fromJson(data);
        auto jsonObj = jsonDocument.object();
        auto pluginsArray = jsonObj[Config::KEY_PLUGINS].toArray();
        for (auto pluginValue : pluginsArray) {
            auto pluginObj = pluginValue.toObject();
            pluginsInfo.insert(pluginObj[Config::KEY_NAME].toString(), pluginObj[Config::KEY_VERSION].toString());
        }
        for (auto itr = newModulesInfo.begin(); itr != newModulesInfo.end(); ++itr) {
            pluginsInfo[itr.key()] = itr.value();
        }
        QJsonArray newPluginArray;
        for (auto itr = pluginsInfo.begin(); itr != pluginsInfo.end(); ++itr) {
            if (itr.key() == "launcher") {
                jsonObj[Config::KEY_LAUNCHER] = itr.value();
            } else if (itr.key() == "mainFrame") {
                jsonObj[Config::KEY_MAINFRAME] = itr.value();
            } else {
                QJsonObject pluginObj;
                pluginObj[Config::KEY_NAME] = itr.key();
                pluginObj[Config::KEY_VERSION] = itr.value();
                newPluginArray.append(pluginObj);
            }
        }
        jsonObj[Config::KEY_PLUGINS] = newPluginArray;
        jsonDocument.setObject(jsonObj);
        file.resize(0);
        file.write(jsonDocument.toJson());
        file.close();
    } else {
        LOG(ERROR) << "open config failed";
    }
}

bool Config::loadUpgradeSelection(QString &remoteVersion, QMap<QString, QJsonObject> &modules)
{
    QFile upgradeSelectionFile(FOLDER_UPGRADE + "/" +FILE_UPGRADE_SELECTION);
    if (!upgradeSelectionFile.exists()) {
        return false;
    }
    if (upgradeSelectionFile.open(QFile::ReadOnly)) {
        auto data = upgradeSelectionFile.readAll();
        auto jb = QJsonDocument::fromJson(data).object();
        remoteVersion = jb[Upgrade::KEY_VERSION].toString();
        modules.clear();
        QJsonArray modulesArray = jb[Upgrade::KEY_MODULES].toArray();
        for (auto moduleValue : modulesArray) {
            auto moduleObj = moduleValue.toObject();
            modules.insert(moduleObj[Upgrade::KEY_MODULENAME].toString(), moduleObj);
        }
        upgradeSelectionFile.close();
    } else {
        LOG(ERROR) << "open upgradeSelection.json failed";
        return false;
    }
}

void Config::saveUpgradeSelection(const QString &remoteVersion, const QMap<QString, QJsonObject> &modules)
{
    QDir upgradeDir(FOLDER_UPGRADE);
    if (!upgradeDir.exists()) {
        upgradeDir.mkdir(".");
    }
    QFile upgradeSelectionFile(FOLDER_UPGRADE + "/" +FILE_UPGRADE_SELECTION);
    if (upgradeSelectionFile.open(QFile::ReadWrite)) {
        QJsonObject jb;
        jb[Upgrade::KEY_VERSION] = remoteVersion;
        QJsonArray modulesArray;
        for (auto itr = modules.begin(); itr != modules.end(); ++itr) {
            modulesArray.append(itr.value());
        }
        jb[Upgrade::KEY_MODULES] = modulesArray;
        QJsonDocument jsonDocument;
        jsonDocument.setObject(jb);
        upgradeSelectionFile.write(jsonDocument.toJson());
        upgradeSelectionFile.close();
    } else {
        LOG(ERROR) << "open upgradeSelection.json failed";
    }
}

void Config::clearUpgradeSelection()
{
    QFile upgradeSelectionFile(FOLDER_UPGRADE + "/" +FILE_UPGRADE_SELECTION);
    upgradeSelectionFile.remove();
}
