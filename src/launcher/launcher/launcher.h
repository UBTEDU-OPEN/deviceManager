#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "upgrade.h"
#include "commonMacro.h"

#include <QWidget>
#include <QMap>
#include <QString>
#include <QByteArray>
#include <QThread>
#include <QList>

class UpgradeProcessor;
class QTranslator;

QT_BEGIN_NAMESPACE
namespace Ui { class Launcher; }
QT_END_NAMESPACE

class Launcher : public QWidget
{
    Q_OBJECT

public:
    Launcher(QWidget *parent = nullptr);
    ~Launcher();

    void setLanguage(int lan);

private:
    void loadSoftwareInfo();
    void startMainFrame();
    void startUpgrade(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &modules);
    void onCancelUpgradeClicked();

    void onGroupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo);
    void onUpgradeProgress(int progress);
    void onDownloadFinished();
    void onInstallFinished();
    void onUpgradeCanceled();
    void onDownloadError();

protected:
    void loadStyleSheet();

signals:
    void upgradeModules(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &modules);
    void stopDownloadModules();

private:
    Ui::Launcher *ui;
    int                         lan_;
    QList<QTranslator*>         translators_;

    QThread                     *processThread_;
    UpgradeProcessor            *upgradeProcessor_;
    QString                     mainFrameVersion_;
    QMap<QString, QString>      modulesInfo_;
    QString                     status_;
    bool                        downloadFailed_;
};
#endif // LAUNCHER_H
