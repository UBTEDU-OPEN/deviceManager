#include "upgrade.h"
#include "logHelper.h"
#include "cmd5.h"
#include "settings.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QDateTime>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include <QJsonObject>
#include <QPluginLoader>
#include <QString>
#include <QFileInfo>

const QString Upgrade::KEY_NAME            { "name" };
const QString Upgrade::KEY_RELEASETIME     { "releaseTime" };
const QString Upgrade::KEY_SIZE            { "size" };
const QString Upgrade::KEY_UPDATELOG       { "updateLog" };
const QString Upgrade::KEY_VERSION         { "version" };
const QString Upgrade::KEY_FORCED          { "forced" };
const QString Upgrade::KEY_MODULES         { "modules" };
const QString Upgrade::KEY_MODULENAME      { "moduleName" };
const QString Upgrade::KEY_PACKAGEMD5      { "packageMd5" };
const QString Upgrade::KEY_PACKAGESIZE     { "packageSize" };
const QString Upgrade::KEY_PACKAGEURL      { "packageUrl" };
const QString Upgrade::KEY_RELEASENOTE     { "releaseNote" };
const QString Upgrade::KEY_VERSIONNAME     { "versionName" };
const QString Upgrade::KEY_INCREMENTMD5    { "incrementMd5" };
const QString Upgrade::KEY_INCREMENTSIZE   { "incrementSize" };
const QString Upgrade::KEY_INCREMENTURL    { "incrementUrl" };
const QString Upgrade::KEY_ISINCREMENTAL   { "isIncremental" };

Upgrade *Upgrade::instance_ = nullptr;

Upgrade::Upgrade(QObject *parent)
    : QObject(parent)
{
    appId_ = "980010105";
    productName_ = "uTools_HM";
    deviceId_ = deviceId();
    networkAccessManager_ = new QNetworkAccessManager(this);
    connect(networkAccessManager_, &QNetworkAccessManager::finished, this, &Upgrade::onReplyFinished);
}

Upgrade *Upgrade::instance()
{
    if (!instance_) {
        instance_ = new Upgrade;
    }
    return instance_;
}

int Upgrade::versionCmp(const QString &localVersion, const QString &serverVersion)
{
    if (localVersion.isNull() || localVersion.isEmpty()) {
        return -1;
    }
    if (localVersion.isNull() || localVersion.isEmpty()) {
        return 1;
    }
    QStringList localVersionLst = localVersion.split(".");
    QStringList serverVersionLst = serverVersion.split(".");
    for (int i = 0; i < qMin(localVersionLst.count(), serverVersionLst.count()); ++i) {
        if (localVersionLst[i].toInt() < serverVersionLst[i].toInt()) {
            return -1;
        } else if (localVersionLst[i].toInt() > serverVersionLst[i].toInt()) {
            return 1;
        }
    }
    if (localVersionLst.count() < serverVersionLst.count()) {
        return -1;
    }
    if (localVersionLst.count() > serverVersionLst.count()) {
        return 1;
    }
    return 0;
}

void Upgrade::requestModulesInfo(const QString &/*languageName*/)
{
    auto url = Settings::upgradeUrl() + "version/available" +
               "?productName=" + productName_ /*+
               "&languageName=" + languageName*/;
    LOG(INFO) << "Upgrade::requestModulesInfo url=" << url.toStdString();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    setHttpHeader(request);
    auto reply = networkAccessManager_->get(request);
    reply2req_.insert(reply, ModulesInfo);
}

void Upgrade::requestGroupUpgradable(const QString &/*languageName*/, const QMap<QString, QString> &modulesName2version)
{
    auto url = Settings::upgradeUrl() + "version/new/upgradable" +
               "?moduleNames=" + modulesName2version.keys().join(",") +
               "&productName=" + productName_ +
               "&versionNames=" + modulesName2version.values().join(",") /*+
               "&languageName=" + languageName*/;
    LOG(INFO) << "Upgrade::requestGroupUpgradable url=" << url.toStdString();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    setHttpHeader(request);
    auto reply = networkAccessManager_->get(request);
    reply2req_.insert(reply, RequestGroupUpgradableByModulesInfo);
}

void Upgrade::requestGroupUpgradable(const QString &/*languageName*/, const QString &groupVersion)
{
    auto url = Settings::upgradeUrl() + "group/upgradable" +
               "?productName=" + productName_ +
               "&groupVersion=" + groupVersion /*+
               "&languageName=" + languageName*/;
    LOG(INFO) << "Upgrade::requestGroupUpgradable url=" << url.toStdString();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    setHttpHeader(request);
    auto reply = networkAccessManager_->get(request);
    reply2req_.insert(reply, RequestGroupUpgradableByGroupVersion);
}

void Upgrade::download(const QString &moduleName, const QString &url, int startPos)
{
    QNetworkRequest request;
    auto destFilePath = Upgrade::downloadFileUrl2DestPath(url);
    request.setUrl(url);
    if (startPos > 0) {
        QString range = QString("bytes=%1-").arg(startPos);
        request.setRawHeader("Range", range.toLatin1());
    }
    auto reply = networkAccessManager_->get(request);
    reply->setProperty("moduleName", moduleName);
    reply->setProperty("destFilePath", destFilePath);
    connect(reply, &QNetworkReply::readyRead, this, &Upgrade::onDownloadReadyRead);
    connect(reply, &QNetworkReply::downloadProgress, this, &Upgrade::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, &Upgrade::onDownloadFinished);
    reply2req_.insert(reply, DownloadFile);
}

void Upgrade::stopDownload()
{
    QDir upgradeFolder(Settings::upgradeFolderAbsPath());
    if (upgradeFolder.exists()) {
        upgradeFolder.removeRecursively();
    }
    auto downloadReplies = reply2req_.keys(DownloadFile);
    while (downloadReplies.count() != 0) {
        auto downloadReply = downloadReplies.takeFirst();
        disconnect(downloadReply, nullptr, this, nullptr);
        downloadReply->abort();
        downloadReply->deleteLater();
    }
}
QString Upgrade::downloadFileUrl2DestPath(const QString &urlStr)
{
    auto url = QUrl(urlStr);
    auto destFilePath = QFileInfo(Settings::upgradeFolderAbsPath() + "/" + url.fileName()).absoluteFilePath();
    return destFilePath;
}

void Upgrade::requestUpgradeFeedback()
{
    auto url = Settings::upgradeUrl() + "feedback";
    LOG(INFO) << "Upgrade::requestUpgradeFeedback url=" << url.toStdString();
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    setHttpHeader(request);
    auto reply = networkAccessManager_->get(request);
    reply2req_.insert(reply, UpgradeFeedback);
}

void Upgrade::addReplyProcessor(UpgradeReplyProcessor *processor)
{
    replyProcessors_.insert(processor);
}

void Upgrade::removeReplyProcessor(UpgradeReplyProcessor *processorInst)
{
    replyProcessors_.remove(processorInst);
}

QString Upgrade::deviceId()
{
    return "ubt-aoduan";
}


void Upgrade::onReplyFinished(QNetworkReply *reply)
{
    LOG(INFO) << "Upgrade::onReplyFinished:";
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    int networkError = reply->error();
    LOG(INFO) << "\thttpStatusCode:" << httpStatusCode;
    LOG(INFO) << "\tnetworkError:" << networkError;
//    bool succ = (httpStatusCode == 200 && networkError == QNetworkReply::NoError);
    QByteArray bytes = reply->readAll();
    QString replyStr = QString::fromUtf8(bytes);
    LOG(INFO) << "\trequest url:" << reply->request().url().toString().toStdString();
    LOG(INFO) << "\treply:" << replyStr.toStdString();
    if (reply2req_.contains(reply)) {
        auto requestType = reply2req_.value(reply);
        LOG(INFO) << "\trequest type:" << requestType;
        for (auto itr = replyProcessors_.begin(); itr != replyProcessors_.end(); ++itr) {
            (*itr)->replyCallback(httpStatusCode, networkError, requestType, bytes);
        }
    }
    reply2req_.remove(reply);
    reply->deleteLater();
}

void Upgrade::onDownloadReadyRead()
{
    if (auto downloadReply = dynamic_cast<QNetworkReply*>(sender())) {
        auto destFilePath = downloadReply->property("destFilePath").toString();
        if (destFilePath.isEmpty()) {
            LOG(ERROR) << "dest file is empty";
            return;
        }
        QFileInfo destFileInfo(destFilePath);
        if (!destFileInfo.dir().exists()) {
            destFileInfo.dir().mkdir(".");
        }
        QFile destFile(destFilePath);
        if (destFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            destFile.write(downloadReply->readAll());
        }
        destFile.close();
    } else {
        LOG(ERROR) << "sender is not QNetworkRelpy";
    }
}

void Upgrade::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (auto downloadReply = dynamic_cast<QNetworkReply*>(sender())) {
        auto moduleName = downloadReply->property("moduleName").toString();
        if (moduleName.isEmpty()) {
            LOG(ERROR) << "moduleName is empty";
            return;
        }
        emit downloadProgress(moduleName, bytesReceived, bytesTotal);
    } else {
        LOG(ERROR) << "sender is not QNetworkRelpy";
    }
//    LOG(INFO) << "Upgrade::onDownloadProgress " << bytesReceived << " " << bytesTotal;
}

void Upgrade::onDownloadFinished()
{
    if (auto downloadReply = dynamic_cast<QNetworkReply*>(sender())) {
        LOG(WARNING) << downloadReply->errorString().toStdString() << downloadReply->error();
        if(QNetworkReply::NoError == downloadReply->error()) {
            auto moduleName = downloadReply->property("moduleName").toString();
            if (moduleName.isEmpty()) {
                LOG(ERROR) << "moduleName is empty";
                return;
            }
            emit downloadFinished(moduleName);
        } else {
//            stopDownload();
            emit downloadError();
        }

    } else {
        LOG(ERROR) << "sender is not QNetworkRelpy";
    }
}

void Upgrade::setHttpHeader(QNetworkRequest &request, bool needAuthorization)
{
    auto sign = QString::fromStdString(ubtSign_.getHeaderXUBTSignV3(deviceId_.toStdString()));
    LOG(INFO) << "Upgrade::setHttpHeader:";
    LOG(INFO) << "\tsign:" << sign.toStdString();
    LOG(INFO) << "\tappId:" << appId_.toStdString();
    LOG(INFO) << "\tdeviceId:" << deviceId_.toStdString();
    LOG(INFO) << "\tneedAuthorization:" << needAuthorization;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");
    request.setRawHeader("X-UBT-Sign", QByteArray::fromStdString(sign.toStdString()));
    request.setRawHeader("X-UBT-AppId", QByteArray::fromStdString(appId_.toStdString()));
    request.setRawHeader("X-UBT-DeviceId", QByteArray::fromStdString(deviceId_.toStdString()));
//    if (needAuthorization) {
//        request.setRawHeader("authorization", QByteArray::fromStdString(authorization.toStdString()));
//    }
}

