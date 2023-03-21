#ifndef DEVPLUGININTERFACE_H
#define DEVPLUGININTERFACE_H

#include "pluginGlobal.h"

#include <QString>
#include <QWidget>
#include <QObject>
#include <QThread>
#include <QSet>
#include <QTabWidget>
#include <QTranslator>
#include <QJsonObject>

namespace ClassroomFileKey
{
const QString DeviceTypeName = "device_type_name";
const QString AutomaticSorting = "automatic_sorting";
const QString Devices        = "devices";
const QString DeviceID       = "id";
const QString DeviceSN       = "sn";
const QString DevicePosX     = "position_x";
const QString DevicePosY     = "position_y";
const QString DeviceStatus   = "status";
}

namespace UpgradePackageInfoKey {
const QString DisplayName     = "display_name";
const QString UpgradeFileName = "upgrade_file_name";
const QString UpgradeFileSize = "upgrade_file_size";
const QString Version         = "version";
const QString Md5             = "md5";
const QString Config          = "config";
const QString Description     = "description";
const QString DiffPackage     = "diff_package";
}

int PLUGIN_EXPORT versionCmp(QString ver1, QString ver2);

class QPushButton;

class PLUGIN_EXPORT DevPluginUI : public QWidget
{
    Q_OBJECT

public:
    explicit DevPluginUI(QWidget *parent) : QWidget(parent) {}

    virtual void load(const QString &filePath) = 0;
    virtual QString save(const QString &filePath) = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual bool deviceExecuting() = 0;
    virtual QSet<QString> allDevicesSNs() const = 0;

    void setContainer(QTabWidget *container) { container_ = container; }
    QTabWidget *container() const { return container_; }

private:
    QTabWidget *container_;

signals:
    void activateRequest();
};

class PLUGIN_EXPORT PackageImportThreadInterface : public QThread
{
    Q_OBJECT

public:
    explicit PackageImportThreadInterface(const QString &/*packagePath*/,  const QString &deviceType, QObject *parent = nullptr)
        : QThread(parent), deviceType_(deviceType) {}

    virtual void stopImport() = 0;

signals:
    void importStart();
    void importProcessing(int value);
    void importFinished(bool success, QString errorStr);
    void importCanceled();

protected:
    QString deviceType_;
};

class PLUGIN_EXPORT ConfirmProcedure : public QObject
{
    Q_OBJECT

public:
    explicit ConfirmProcedure(QObject *parent = nullptr) : QObject(parent) {}

    virtual void startProcedure() = 0;
    virtual void hideSaveUserData(){}

signals:
    void confirmComplete(bool accepted, int mode);
};

enum Language {
    Unset = -1,
    Cn = 0,
    En = 1,
};

struct PLUGIN_EXPORT UpgradePackageInfo {
    QString displayName;
    QString upgradeFileName;
    QString version;
    QString md5;
    double upgradeFileSize;
    QString description;
    bool diffPackage;

    QJsonObject config;
};

class PLUGIN_EXPORT DevPluginInterface
{
public:
    enum ImageType {
        Normal,
        Hover,
        Pressed,
        Disable,
        CountNum
    };

    enum UpgradeCondition {
        CmdConnect         = 1,
        Charging           = 2,
        Version            = 4,
        LockStatus         = 8,
    };

public:
    DevPluginInterface();
    virtual ~DevPluginInterface() = default;

    virtual void setLanguage(Language lan) = 0;

    virtual DevPluginUI* createUI(const QString &filePath) const;
    void load(const QString &f, DevPluginUI *ui) const;
    void save(const QString &f, DevPluginUI *ui) const;

    void setAuthor(const QString &author);
    QString author() const;

    void setDate(const QString &date);
    QString date() const;

    void setName(const QString &name);
    QString name() const;

    void setVersion(const QString &ver);
    QString version() const;

    void setDeviceType(int32_t type);
    int32_t deviceType() const;

    virtual QStringList deviceTypeSelectionImagesUrls() const = 0;
    virtual QStringList deviceItemImagesUrls() const = 0;
    virtual int cmdExcuteEstimateTime() const = 0;

    // upgrade
    bool readUpgradePackageInfo(UpgradePackageInfo &upgradePackageInfo) const;
    bool writeUpgradePackageInfo(const UpgradePackageInfo &upgradePackageInfo) const;
    virtual PackageImportThreadInterface* createPackageImportThread(const QString &packagePath, QObject *parent) const = 0;
    virtual ConfirmProcedure* createUpgradeConfirmProcedure(QWidget *parent) const = 0;
    virtual bool upgradeVersionCheck(QString deviceVersion, const UpgradePackageInfo &upgradePackageInfo, QString &upgradeConfirmNotice) const = 0;
    bool meetUpgradeRequirements(int conditions) const;
    bool testUpgradeRequirements(int condition) const;
    virtual QString upgradeConditionsDescription(int conditions, const UpgradePackageInfo &upgradePackageInfo) const = 0;
    virtual QString upgradeNoticeText() const = 0;
    virtual QString upgradeSuccessText() const = 0;

    int upgradeTimeLimit() const;                       // 单设备最长升级时间（从传包完成计时），超过视为升级超时
    int parallelTransferDeviceCountLimit() const;       // 同时传输设备最大数目
    int transferAttemptionTimesLimit() const;           // 单设备最大传输尝试次数
    int transferContinuousAttemptionTimesLimit() const; // 单设备连续传输尝试次数

    // reset
    virtual ConfirmProcedure* createResetConfirmProcedure(QWidget *parent) const = 0;
    virtual ConfirmProcedure* createShutDownProcedure(QWidget *parent) const = 0;

    // lock
    virtual bool exclusiveLock() const = 0;

    virtual bool obsoleteVersion(const QString &deviceVersion) const = 0;

protected:
    QString settingsPath() const;
    QString upgradePkgInfoPath() const;

protected:
    QString     author_;
    QString     date_;
    QString     name_;
    QString     version_;
    int32_t     deviceType_;
    int         upgradeRequirements_;

    Language    lan_;
    QTranslator *translator_;

    bool        oldVersionPkg_;
};

#define DevPluginInterface_iid "ubt.DevPluginInterface/1.0"
Q_DECLARE_INTERFACE(DevPluginInterface, DevPluginInterface_iid)

#endif // DEVPLUGININTERFACE_H
