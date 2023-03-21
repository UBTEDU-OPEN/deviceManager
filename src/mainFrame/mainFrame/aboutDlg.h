#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>

#include "upgrade.h"

namespace Ui {
class AboutDlg;
}

class AboutDlg : public QDialog, public UpgradeReplyProcessor
{
    Q_OBJECT

public:
    explicit AboutDlg(QString version, QWidget *parent = nullptr);
    ~AboutDlg();

    void replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData) override;

protected:
    void loadStyleSheet();

    void mousePressEvent(QMouseEvent *event) override;
    void onTimeout();
    void onCheckVersion();

private:
    void onIgnored();
    void onUpgrade(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &upgradeModules);
    void onGroupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo);

private:
    Ui::AboutDlg *ui;

    QString version_;
    QTimer  *timer_;
    int     clickCount_;
    Upgrade *upgrade_;
    QString remoteGrpVersion_;
    QMap<QString, QJsonObject> modulesToUpgrade_;
};

#endif // ABOUTDLG_H
