#include "aboutDlg.h"
#include "ui_aboutDlg.h"

#include "logHelper.h"
#include "config.h"
#include "restartToUpgradeDlg.h"
#include "upgradableModulesSelector.h"
#include "commondialog.h"
#include "settings.h"
#include "toastdialog.h"

#include <QDesktopServices>
#include <QProcess>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

AboutDlg::AboutDlg(QString version, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg),
    timer_(nullptr),
    version_(version),
    upgrade_(nullptr),
    clickCount_(0)
{
    ui->setupUi(this);
    loadStyleSheet();

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->PrivacyLabel->setText(tr("<a href=\"https://gdpr.ubtrobot.com/utools_hm/privacy.html\">privacy</a>"));
    connect(ui->PrivacyLabel, &QLabel::linkActivated, this, [this](const QString &url) {
        QDesktopServices::openUrl(url);
    });

    auto pos = version_.lastIndexOf('.');
    ui->VersionLabel->setText(tr("version:") + version_.mid(0, pos));
    ui->VersionCheckBtn->setEnabled(true);

    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_, &QTimer::timeout,this,&AboutDlg::onTimeout);
    connect(ui->VersionCheckBtn, &QPushButton::clicked, this, &AboutDlg::onCheckVersion);

    upgrade_ = new Upgrade(this);
    upgrade_->addReplyProcessor(this);

    ui->warning->setVisible(false);
}

AboutDlg::~AboutDlg()
{
    delete ui;
}

void AboutDlg::replyCallback(int httpStatusCode, int networkError, Upgrade::ReqType reqType, QByteArray replyData)
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

void AboutDlg::loadStyleSheet()
{
    QFile styleSheetFile(":/res/qss/aboutDlg.qss");
    if (styleSheetFile.open(QFile::ReadOnly)) {
        setStyleSheet(styleSheetFile.readAll());
        styleSheetFile.close();
    } else {
        LOG(ERROR) << "open :/res/qss/aboutDlg.qss failed";
    }
}

void AboutDlg::mousePressEvent(QMouseEvent *event)
{
    if (timer_->isActive()) {
        ++clickCount_;
        if (clickCount_ == 6) {
            QMessageBox::information(this,QString::fromLocal8Bit("版本号"), version_);
        }
    } else {
        clickCount_ = 0;
        timer_->start(3000);
    }
    QWidget::mousePressEvent(event);
}

void AboutDlg::onTimeout()
{
    clickCount_ = 0;
}

void AboutDlg::onCheckVersion()
{
    if(upgrade_ == nullptr) {
        LOG(WARNING) << "upgrade_ is null";
        upgrade_ = new Upgrade(this);
        upgrade_->addReplyProcessor(this);
    }
    upgrade_->requestGroupUpgradable("en", Config::groupVersion());
}

void AboutDlg::onIgnored()
{
//    Settings::saveIgnoreGroupVersion(remoteGrpVersion_);
}

void AboutDlg::onUpgrade(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &upgradeModules)
{
    Config::saveUpgradeSelection(remoteGrpVersion, upgradeModules);

#ifdef DEBUG
    QString launcherFilePath = "../../launcher/" + Config::laucherInfo() + "/launcherd.exe";
#else
    QString launcherFilePath = "../../launcher/" + Config::laucherInfo() + "/launcher.exe";
#endif
    QProcess::startDetached(launcherFilePath, QStringList(), "../../launcher/" + Config::laucherInfo());
    exit(0);
}

void AboutDlg::onGroupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo)
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
    if(networkError != 0) {
        ui->warning->setVisible(true);
        upgrade_->deleteLater();
        upgrade_ = nullptr;
        return;
    } else {
        ui->warning->setVisible(false);
    }
    remoteGrpVersion_ = remoteGrpInfo.value(Upgrade::KEY_VERSION).toString();
    if (remoteGrpVersion_.isNull() || remoteGrpVersion_.isEmpty()) {
        LOG(INFO) << "invalid remote group version";
        auto dialog = new ToastDialog(this);
        dialog->setDisplayText(tr("Already the latest version"));
        dialog->move((width()-dialog->width())/2,(height()-dialog->height())/2);
        dialog->show();
        ui->VersionCheckBtn->setText(tr("latest version"));
        return;
    }


    QMap<QString, QJsonObject> modulesToUpgrade;
    auto modulesArray = remoteGrpInfo.value(Upgrade::KEY_MODULES).toArray();
    auto modulesInfo = Config::modulesInfo();
    for (auto modulesValue : modulesArray) {
        auto modulesObj = modulesValue.toObject();
        if (!modulesObj.isEmpty()) {
            auto moduleName = modulesObj.value(Upgrade::KEY_MODULENAME).toString();
            if (modulesInfo.contains(moduleName)) {
                if (Upgrade::versionCmp(modulesInfo.value(moduleName), modulesObj.value(Upgrade::KEY_VERSIONNAME).toString()) == -1) {
                    LOG(INFO) << "lower module version, local:" << modulesInfo.value(moduleName).toStdString() << " remote:" << modulesObj.value(Upgrade::KEY_VERSIONNAME).toString().toStdString();
                    modulesToUpgrade.insert(moduleName, modulesObj);
                }
            } else {
                modulesToUpgrade.insert(moduleName, modulesObj);
                LOG(INFO) << "new module, module name:" << moduleName.toStdString();
            }
        }
    }

    if (Upgrade::versionCmp(Config::groupVersion(), remoteGrpVersion_) == 0 && modulesToUpgrade.count() == 0) {
        LOG(INFO) << "remote version " << remoteGrpVersion_.toStdString() << " is as same as local version";
        ui->VersionCheckBtn->setText(tr("latest version"));
        return;
    }

    CommonDialog *dialog = new CommonDialog("", CommonDialog::NoButton, this);
    auto updateLog = remoteGrpInfo.value(Upgrade::KEY_UPDATELOG).toString();
    auto upgradableModulesSelector = new UpgradableModulesSelector(remoteGrpVersion_, updateLog, modulesToUpgrade, this);
    connect(upgradableModulesSelector, &UpgradableModulesSelector::upgrade, this, &AboutDlg::onUpgrade);
    connect(upgradableModulesSelector, &UpgradableModulesSelector::ignored, this, &AboutDlg::onIgnored);
    connect(upgradableModulesSelector, &UpgradableModulesSelector::upgrade, dialog, &CommonDialog::close);
    connect(upgradableModulesSelector, &UpgradableModulesSelector::ignored, dialog, &CommonDialog::close);
    dialog->setDisplayWidget(upgradableModulesSelector);
    dialog->exec();
}
