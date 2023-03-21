#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "devicePluginInterface.h"

#include <QDialog>
#include <QMap>
#include <QWidget>

class QPluginLoader;
class QVBoxLayout;

class PluginInfo : public QWidget
{
    Q_OBJECT

public:
    PluginInfo(const QString &pluginName, const QString &localVersion,
               const QString &url, const QString &remoteVersion,
               QWidget *parent = nullptr);
    ~PluginInfo();

private:
    QString url_;
};

class PluginManager : public QDialog
{
    Q_OBJECT

public:
    QMap<QString, DevPluginInterface*> plugins;

public:
    PluginManager(QWidget *parent = nullptr);
    ~PluginManager();

    void loadPlugins();
    DevPluginUI* load(const QString &classroomFile);
    DevPluginUI* createNewClassroom(const QString& platformName);

    void setLanguage(Language lan);

private:
    void addPluginInfo(const QString &pluginName, const QString &localVersion,
                       const QString &remoteURL, const QString &remoteVersion);

private:
    QVBoxLayout *mainLayout_;
};
#endif // PLUGINMANAGER_H
