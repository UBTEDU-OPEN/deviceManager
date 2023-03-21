#ifndef AIBOXPLUGIN_H
#define AIBOXPLUGIN_H

#include "aiboxPluginGlobal.h"

#include "devicePluginInterface.h"

#include <QObject>
#include <QString>
#include <QSet>
#include <QByteArray>

class QTranslator;

class AIBOXPLUGIN_EXPORT AiboxPlugin : public QObject, public DevPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ubt.DevPluginInterface" FILE "AiboxPlugin.json")
    Q_INTERFACES(DevPluginInterface)

public:
    explicit AiboxPlugin(QObject *p = nullptr);

    void setLanguage(Language lan) override;

    QStringList deviceTypeSelectionImagesUrls() const override;

    QStringList deviceItemImagesUrls() const override;

    int cmdExcuteEstimateTime() const override;

    PackageImportThreadInterface *createPackageImportThread(const QString &packagePath, QObject *parent) const override;
    ConfirmProcedure* createUpgradeConfirmProcedure(QWidget *parent) const override;
    bool upgradeVersionCheck(QString deviceVersion, const UpgradePackageInfo &upgradePackageInfo, QString &upgradeConfirmNotice) const override;
    QString upgradeNoticeText() const override;
    QString upgradeSuccessText() const override;
    QString upgradeConditionsDescription(int conditions, const UpgradePackageInfo &upgradePackageInfo) const override;

    ConfirmProcedure* createResetConfirmProcedure(QWidget *parent) const override;
    ConfirmProcedure* createShutDownProcedure(QWidget *parent) const override;

    bool exclusiveLock() const override;

    bool obsoleteVersion(const QString &deviceVersion) const override;
};

#endif // AIBOXPLUGIN_H
