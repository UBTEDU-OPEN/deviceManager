#ifndef CONFIG_H
#define CONFIG_H

#include "utilsGlobal.h"

#include <QString>
#include <QMap>
#include <QJsonObject>

class UTILS_EXPORT Config
{
public:
    // folder
    static const QString FOLDER_CONFIG;
    static const QString FOLDER_UPGRADE;
    // file
    static const QString FILE_CONFIG;
    static const QString FILE_UPGRADE_SELECTION;
    // key
    static const QString KEY_GROUP_VERSION;
    static const QString KEY_LAUNCHER;
    static const QString KEY_MAINFRAME;
    static const QString KEY_PLUGINS;
    static const QString KEY_NAME;
    static const QString KEY_VERSION;

public:
    static QString configAbsPath();
    static QString groupVersion();
    static void updateGroupVersion(const QString &newVersion);
    static QString laucherInfo();
    static void updateLauncherInfo(const QString &version);
    static QString mainframeInfo();
    static void updateMainframeInfo(const QString &version);
    static QMap<QString, QString> pluginsInfo();
    static QMap<QString, QString> modulesInfo();
    static void updatePluginsInfo(const QMap<QString, QString> &newPluginsInfo);
    static void updateModulesInfo(const QMap<QString, QString> &newModulesInfo);
    static bool loadUpgradeSelection(QString &remoteVersion, QMap<QString, QJsonObject> &modules);
    static void saveUpgradeSelection(const QString &remoteVersion, const QMap<QString, QJsonObject> &modules);
    static void clearUpgradeSelection();
};

#endif // CONFIG_H
