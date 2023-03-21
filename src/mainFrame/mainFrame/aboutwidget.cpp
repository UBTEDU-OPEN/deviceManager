#include "aboutwidget.h"
#include "ui_aboutwidget.h"

#include "logHelper.h"
#include "restartToUpgradeDlg.h"
#include "config.h"
#include "settings.h"

#include <QFile>
#include <QMouseEvent>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QProcess>

aboutwidget::aboutwidget(QString version, QWidget *parent) :
    QWidget(parent),
    m_version(version),
    ui(new Ui::aboutwidget),
    clickCount_(0),
    upgrade_(nullptr)
{
    ui->setupUi(this);
    setFixedSize(580, 320);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | windowFlags());
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    QFile styleSheet(":/res/qss/aboutWidget.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    auto pos = version.lastIndexOf('.');
    ui->versionNum->setText(version.mid(0,pos));
    ui->pbupdate->setEnabled(true);

    connect(ui->pbclose, &QPushButton::clicked, this, &aboutwidget::close);
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_,&QTimer::timeout,this,&aboutwidget::onTimeout);
    connect(ui->pbupdate, &QPushButton::clicked, this, &aboutwidget::onCheckVersion);

    upgrade_ = new Upgrade(this);
    upgrade_->addReplyProcessor(this);
}

aboutwidget::~aboutwidget()
{
    delete ui;
}

void aboutwidget::replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData)
{
    switch(reqType) {
    case Upgrade::RequestGroupUpgradableByGroupVersion: {
        onGroupUpgradableQueryReply(httpStatusCode, networkError, QJsonDocument::fromJson(replyData).object());
    }
        break;
    case Upgrade::DownloadFile: {
    }
        break;
    default: {
        LOG(WARNING) << "unknown request type:" << reqType;
        break;
    }
    }
}

void aboutwidget::onIgnored()
{
    ui->pbupdate->setText(tr("latest version"));
}

void aboutwidget::onGroupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo)
{
    /*
    {
        "forced": false,
        "modules": [
            {
                "incrementMd5": null,
                "incrementSize": null,
                "incrementUrl": null,
                "isIncremental": false,
                "moduleName": "aiboxPlugin",
                "packageMd5": "9cc57ac791b8087f53074a7e7c19bf4a",
                "packageSize": 157092,
                "packageUrl": "https://ubtrobot-new.oss-cn-shenzhen.aliyuncs.com/upgrade-test/2021/07/05/1625451102102/aiboxPlugin_9cc57ac791b8087f53074a7e7c19bf4a_1.1.0.1.gz",
                "releaseNote": "comment:1.1.0.1",
                "versionName": "1.1.0.1"
            },
            {
                "incrementMd5": null,
                "incrementSize": null,
                "incrementUrl": null,
                "isIncremental": false,
                "moduleName": "yansheePlugin",
                "packageMd5": "779ca061f3d1b88694407554a8e11e85",
                "packageSize": 4156781,
                "packageUrl": "https://ubtrobot-new.oss-cn-shenzhen.aliyuncs.com/upgrade-test/2021/07/05/1625451513248/yansheePlugin_755360c21d2d11204aa3bdb93b5432d2_2.0.0.2.zip",
                "releaseNote": "comment:2.0.0.2",
                "versionName": "2.0.0.2"
            },
            {
                "incrementMd5": null,
                "incrementSize": null,
                "incrementUrl": null,
                "isIncremental": false,
                "moduleName": "launcher",
                "packageMd5": "48b57be45f2d50f612ad7c77b587e084",
                "packageSize": 268190,
                "packageUrl": "https://ubtrobot-new.oss-cn-shenzhen.aliyuncs.com/upgrade-test/2021/07/05/1625451631543/launcher_48b57be45f2d50f612ad7c77b587e084_1.0.0.1.rar",
                "releaseNote": "comment:1.0.0.1",
                "versionName": "1.0.0.1"
            },
            {
                "incrementMd5": null,
                "incrementSize": null,
                "incrementUrl": null,
                "isIncremental": false,
                "moduleName": "mainFrame",
                "packageMd5": "fd96cd59e0c2d7338fa4166a9d373f30",
                "packageSize": 3345576,
                "packageUrl": "https://ubtrobot-new.oss-cn-shenzhen.aliyuncs.com/upgrade-test/2021/07/05/1625451953971/mainFrame_76a0db92ad298d9932e97040e854c6e8_1.1.7.3.zip",
                "releaseNote": "comment:1.1.7.3",
                "versionName": "1.1.7.3"
            }
        ],
        "name": "grp1",
        "releaseTime": 1625472700,
        "size": 7927639,
        "updateLog": "group 1",
        "version": "1.0.0.1"
    }
    */
    remoteGrpVersion_ = remoteGrpInfo.value(Upgrade::KEY_VERSION).toString();
    if (remoteGrpVersion_.isNull() || remoteGrpVersion_.isEmpty()) {
        LOG(INFO) << "invalid remote group version";
        ui->pbupdate->setText(tr("latest version"));
        return;
    }
    if (Upgrade::versionCmp(Config::groupVersion(), remoteGrpVersion_) == 0) {
        LOG(INFO) << "remote version " << remoteGrpVersion_.toStdString() << " is as same as local version";
        ui->pbupdate->setText(tr("latest version"));
        return;
    }

    QMap<QString, QJsonObject> modulesToUpgrade;
    auto modulesArray = remoteGrpInfo.value(Upgrade::KEY_MODULES).toArray();
    auto pluginsInfo_ = Config::pluginsInfo();
    for (auto modulesValue : modulesArray) {
        auto modulesObj = modulesValue.toObject();
        if (!modulesObj.isEmpty()) {
            auto moduleName = modulesObj.value(Upgrade::KEY_MODULENAME).toString();
            if (pluginsInfo_.contains(moduleName)) {
                if (Upgrade::versionCmp(pluginsInfo_.value(moduleName), modulesObj.value(Upgrade::KEY_VERSIONNAME).toString()) == -1) {
                    modulesToUpgrade.insert(moduleName, modulesObj);
                }
            } else {
                modulesToUpgrade.insert(moduleName, modulesObj);
            }
        }
    }

    if (modulesToUpgrade.count() != 0) {
        auto restartToUpgradeDlg = new RestartToUpgradeDlg(this);
        connect(restartToUpgradeDlg, &RestartToUpgradeDlg::restartToUpgrade, [this]() {
#ifdef DEBUG
            QString launcherFilePath = "../../launcher/" + Config::laucherInfo() + "/launcherd.exe";
#else
            QString launcherFilePath = "../../launcher/" + Config::laucherInfo() + "/launcher.exe";
#endif
            QProcess::startDetached(launcherFilePath, QStringList(), "../../launcher/" + Config::laucherInfo());
            exit(0);
        });
        restartToUpgradeDlg->exec();
    } else {
        ui->pbupdate->setText(tr("latest version"));
    }
}

void aboutwidget::mousePressEvent(QMouseEvent *event)
{
    if(timer_->isActive()) {
        ++clickCount_;
        if(clickCount_ == 6) {
            QMessageBox::information(this,QString::fromLocal8Bit("版本号"),m_version);
        }
    } else {
        clickCount_ = 0;
        timer_->start(3000);
    }
    QWidget::mousePressEvent(event);
}

void aboutwidget::onTimeout()
{
    clickCount_ = 0;
}

void aboutwidget::onCheckVersion()
{
    if (upgrade_) {
        upgrade_->requestGroupUpgradable("en", Config::groupVersion());
    } else {
        LOG(WARNING) << "upgrade_ is null";
    }
}
