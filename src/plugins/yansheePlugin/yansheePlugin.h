#ifndef YANSHEEPLUGIN_H
#define YANSHEEPLUGIN_H

#include "yansheePluginGlobal.h"
#include "devicePluginInterface.h"

#include <QObject>
#include <QString>
#include <QSet>

class QTranslator;

class YANSHEEPLUGIN_EXPORT YansheePlugin : public QObject, public DevPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ubt.DevPluginInterface" FILE "yansheePlugin.json")
    Q_INTERFACES(DevPluginInterface)

public:
    explicit YansheePlugin(QObject *p = nullptr);

    void setLanguage(Language lan) override;

    QStringList deviceTypeSelectionImagesUrls() const override;

    QStringList deviceItemImagesUrls() const override;

    PackageImportThreadInterface *createPackageImportThread(const QString &packagePath, QObject *parent) const override;
    ConfirmProcedure* createUpgradeConfirmProcedure(QWidget *parent) const override;
    bool upgradeVersionCheck(QString deviceVersion, const UpgradePackageInfo &upgradePackageInfo, QString &upgradeConfirmNotice) const override;
    int cmdExcuteEstimateTime() const override;
    QString upgradeNoticeText() const override;
    QString upgradeSuccessText() const override;
    QString upgradeConditionsDescription(int conditions, const UpgradePackageInfo &upgradePackageInfo) const override;

    ConfirmProcedure* createResetConfirmProcedure(QWidget *parent) const override;
    ConfirmProcedure* createShutDownProcedure(QWidget *parent) const override;

    bool exclusiveLock() const override;

    bool obsoleteVersion(const QString &deviceVersion) const override;
};

#endif // YANSHEEPLUGIN_H
