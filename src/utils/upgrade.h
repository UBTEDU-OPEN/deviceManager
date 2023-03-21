#ifndef UPGRADE_H
#define UPGRADE_H

#include "utilsGlobal.h"
#include "ubtsign.h"

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QSet>
#include <QByteArray>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

struct ModuleInfo {
    QString name;
    QString localVersion;
    QString remoteVersion;
    bool isSelected;
};

class UpgradeReplyProcessor;

class UTILS_EXPORT Upgrade : public QObject
{
    Q_OBJECT

public:
    enum ReqType {
        ModulesInfo = 0,
        RequestGroupUpgradableByModulesInfo,
        RequestGroupUpgradableByGroupVersion,
        DownloadFile,
        UpgradeFeedback,
    };

public:
    static const QString KEY_NAME;
    static const QString KEY_RELEASETIME;
    static const QString KEY_SIZE;
    static const QString KEY_UPDATELOG;
    static const QString KEY_VERSION;
    static const QString KEY_FORCED;
    static const QString KEY_MODULES;
    static const QString KEY_MODULENAME;
    static const QString KEY_PACKAGEMD5;
    static const QString KEY_PACKAGESIZE;
    static const QString KEY_PACKAGEURL;
    static const QString KEY_RELEASENOTE;
    static const QString KEY_VERSIONNAME;
    static const QString KEY_INCREMENTMD5;
    static const QString KEY_INCREMENTSIZE;
    static const QString KEY_INCREMENTURL;
    static const QString KEY_ISINCREMENTAL;

public:
    Upgrade(QObject *parent = nullptr);

    static Upgrade *instance();
    static int versionCmp(const QString &localVersion, const QString &serverVersion);

    void requestModulesInfo(const QString &languageName);
    void requestGroupUpgradable(const QString &languageName, const QMap<QString, QString> &modulesName2version);
    void requestGroupUpgradable(const QString &languageName, const QString &groupVersion);
    void download(const QString &moduleName, const QString &url, int startPos);
    void stopDownload();
    static QString downloadFileUrl2DestPath(const QString &urlStr);
    void requestUpgradeFeedback();

    void addReplyProcessor(UpgradeReplyProcessor *processor);
    void removeReplyProcessor(UpgradeReplyProcessor *processor);

private:
    QString deviceId();

    void onReplyFinished(QNetworkReply *reply);

    void onDownloadReadyRead();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

    void setHttpHeader(QNetworkRequest &request, bool needAuthorization = true);

signals:
    void downloadProgress(const QString &moduleName, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &moduleName);
    void downloadError();

private:
    static Upgrade *instance_;

    QMutex mutex;
    QNetworkAccessManager *networkAccessManager_;
    QMap<QNetworkReply*, ReqType> reply2req_;
    QString appId_;
    QString appKey_;
    QString productName_;
    QString deviceId_;
    UBTSign ubtSign_;

    QSet<UpgradeReplyProcessor*> replyProcessors_;
};

class UTILS_EXPORT UpgradeReplyProcessor
{
public:
    virtual void replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData) = 0;
};

#endif // UPGRADE_H
