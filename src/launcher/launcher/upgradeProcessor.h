#ifndef UPGRADEPROCESSOR_H
#define UPGRADEPROCESSOR_H

#include "upgrade.h"

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <QJsonObject>

class QProcess;

class UpgradeProcessor : public QObject, public UpgradeReplyProcessor
{
    Q_OBJECT

public:
    QString                 mainFrameVersion;
    QMap<QString, QString>  pluginsInfo;

public:
    UpgradeProcessor(QObject *parent = nullptr);
    virtual ~UpgradeProcessor();

    void replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData) override;

    void startProgress();

    void requestModuleInfo();
    void requestGrpUpgradableByModulesInfo(const QString &mainFrameVersion);
    void requestGrpUpgradableByGrpVersion();
    void upgradeModules(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &modules);
    void stopDownloadModules();
    bool isInstalling();
    void requestCancelInstall();
    void cancelClicked();
    void cancelCanceled();

    static bool checkLocalUpradePackage(const QJsonObject &moduleObj, int &startPos, qint64 &totalSize);

private:
    void onRequestGrpUpgradableByGrpVersionReply(int httpStatusCode, int networkError, const QByteArray &replyData);
    void onDownloadProgress(const QString &moduleName, qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(const QString &moduleName);
    void installModules();
    void cleanObsoleteVersion(const QMap<QString, QString> &upgradeModules2version);
    void onDownloadError();
    void removeUpgradeFolder();
    void createNewLink(QString newVersion);

signals:
    void onReply(const QString &replyStr);
    void groupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo);

    void startUpgrade(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &modules);
    void downloadProgress(int progress);
    void downloadFinished();
    void installProgress(int progress);
    void installFinished();
    void upgradeCanceled();
    void downloadError();

private:
    Upgrade                     *upgrade_;
    QString                     remoteGrpVersion_;
    QMap<QString, QJsonObject>  modulesToDownload_;
    QMap<QString, QJsonObject>  modulesToInstall_;
    qint64                      upgradeModulesSize_;
    qint64                      alreadyDownloadSize_;
    QMap<QString, qint64>       downloadedUpgradeModulesSize_;
    QProcess                    *extractProcess_;
    bool                        cancelInstall_;
    bool                        installing_;
    bool                        preCancel_;
    QString                     msg_;
};

#endif // UPGRADEPROCESSOR_H
