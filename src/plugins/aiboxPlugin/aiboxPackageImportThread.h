#ifndef AIBOXPACKAGEIMPORTTHREAD_H
#define AIBOXPACKAGEIMPORTTHREAD_H

#include "aiboxPluginGlobal.h"

#include "devicePluginInterface.h"

#include <QString>

class QProcess;

class AIBOXPLUGIN_EXPORT AiboxPackageImportThread : public PackageImportThreadInterface
{
    Q_OBJECT

public:
    static const QString CONFIG_KEY_VERSION;
    static const QString CONFIG_KEY_MD5;
    static const QString CONFIG_KEY_ALL_MD5;
    static const QString CONFIG_KEY_DELTA_MD5;
    static const QString CONFIG_KEY_FROM_VERSION;
    static const QString CONFIG_KEY_TO_VERSION;

public:
    explicit AiboxPackageImportThread(const DevPluginInterface *plugin, const QString &packagePath, const QString &deviceType, QObject *parent = nullptr);

    void run() override;
    void stopImport() override;

private:
    bool cancelCheckPoint(int value);
    bool configJson();

private:
    bool cancelRequest_;

    QString packagePath_;
    QProcess *extractProcess_;
    UpgradePackageInfo upgradePackageInfo_;
    const DevPluginInterface *plugin_;
};


#endif // AIBOXPACKAGEIMPORTTHREAD_H
