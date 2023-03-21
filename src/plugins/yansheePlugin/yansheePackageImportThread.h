#ifndef YANSHEEPACKAGEIMPORTTHREAD_H
#define YANSHEEPACKAGEIMPORTTHREAD_H

#include "yansheePluginGlobal.h"

#include "devicePluginInterface.h"

#include <QString>
#include <QProcess>

class YANSHEEPLUGIN_EXPORT YansheePackageImportThread : public PackageImportThreadInterface
{
    Q_OBJECT

public:
    static const QString CONFIG_KEY_FILE;
    static const QString CONFIG_KEY_VERSION;
    static const QString CONFIG_KEY_MD5;
    static const QString CONFIG_KEY_FROM_VERSION_MIN;

public:
    explicit YansheePackageImportThread(const DevPluginInterface *plugin, const QString &packagePath, const QString &deviceType, QObject *parent = nullptr);

    void run() override;

    void stopImport() override;

private:
    bool cancelCheckPoint(int value);

private:
    bool cancelRequest_;
    QProcess *extractProcess_;
    QString packagePath_;
    UpgradePackageInfo upgradePackageInfo_;
    const DevPluginInterface *plugin_;
};

#endif // YANSHEEPACKAGEIMPORTTHREAD_H
