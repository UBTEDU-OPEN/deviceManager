#ifndef ABOUTWIDGET_H
#define ABOUTWIDGET_H

#include "upgrade.h"

#include <QWidget>
#include <QTimer>
#include <QMap>
#include <QString>

namespace Ui {
class aboutwidget;
}

class aboutwidget : public QWidget, public UpgradeReplyProcessor
{
    Q_OBJECT

public:
    explicit aboutwidget(QString version, QWidget *parent = nullptr);
    ~aboutwidget();

    void replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData) override;

private:
    void onIgnored();
    void onGroupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void onTimeout();
    void onCheckVersion();

private:
    Ui::aboutwidget *ui;
    QString m_version;
    QTimer *timer_;
    int clickCount_;
    Upgrade *upgrade_;
    QString remoteGrpVersion_;
};

#endif // ABOUTWIDGET_H
