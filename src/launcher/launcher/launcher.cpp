#include "launcher.h"
#include "ui_launcher.h"

#include "upgradeProcessor.h"
#include "upgradableModulesSelector.h"
#include "cancelUpgradeConfirmDlg.h"
#include "commondialog.h"

#include "logHelper.h"
#include "settings.h"
#include "config.h"
#include "commonMacro.h"

#include <QTranslator>
#include <QProcess>
#include <QLibrary>
#include <QTimer>
#include <QScreen>
#include <QFile>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QStyle>

QString LauncherVersion { STR_MACRO(LAUNCHER_VERSION) };

Launcher::Launcher(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Launcher)
    , lan_(-1)
    , processThread_(nullptr)
    , upgradeProcessor_(nullptr)
    , downloadFailed_(false)
{
    LOG(WARNING) << "################### Launcher Version:" << LauncherVersion.toStdString();

    setLanguage(0);

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);
    loadStyleSheet();

    ui->ProgressBar->setMinimum(0);
    ui->ProgressBar->setMaximum(100);
    ui->ProgressBar->setValue(0);
    ui->ProgressBar->hide();
    ui->StatusLabel->hide();
    ui->CancelBtn->hide();

    auto screenSize = screen()->geometry();
    auto w = (qreal(1280) / 1920) * screenSize.width();
    auto h = (qreal(769) / 1080) * screenSize.height();
    setFixedSize(w, h);

    loadSoftwareInfo();
    connect(ui->CancelBtn, &QPushButton::clicked, this, &Launcher::onCancelUpgradeClicked);
}

Launcher::~Launcher()
{
    if (processThread_) {
        processThread_->quit();
        processThread_->wait();
    }
    delete ui;
}

void Launcher::setLanguage(int lan)
{
    LOG(INFO) << "Launcher::setLanguage lan = " << lan;
    if (lan_ != lan) {
        lan_ = lan;
        while (translators_.count() != 0) {
            auto translator = translators_.takeFirst();
            qApp->removeTranslator(translator);
            translator->deleteLater();
            translator = nullptr;
        }
        switch(lan) {
        case 0 /*Cn*/: {
            auto translator = new QTranslator(this);
            if (translator->load("language/launcher_cn") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load launcher_cn succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load launcher_cn failed";
                translator->deleteLater();
                translator = nullptr;
            }
            translator = new QTranslator(this);
            if (translator->load("language/utils") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load utils succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load utils failed";
                translator->deleteLater();
                translator = nullptr;
            }
        }
            break;
        default: {
            auto translator = new QTranslator(this);
            if (translator->load("language/launcher_cn") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load launcher_cn succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load launcher_cn failed";
                translator->deleteLater();
                translator = nullptr;
            }
            translator = new QTranslator(this);
            if (translator->load("language/utils") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load utils succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load utils failed";
                translator->deleteLater();
                translator = nullptr;
            }
        }
            break;
        }
    }
}

void Launcher::loadSoftwareInfo()
{
    mainFrameVersion_ = Config::mainframeInfo();
    modulesInfo_ = Config::modulesInfo();
    processThread_ = new QThread(this);
    upgradeProcessor_ = new UpgradeProcessor;
    upgradeProcessor_->moveToThread(processThread_);
    connect(upgradeProcessor_, &UpgradeProcessor::groupUpgradableQueryReply, this, &Launcher::onGroupUpgradableQueryReply);
    connect(upgradeProcessor_, &UpgradeProcessor::downloadProgress, this, &Launcher::onUpgradeProgress);
    connect(upgradeProcessor_, &UpgradeProcessor::downloadFinished, this, &Launcher::onDownloadFinished);
    connect(upgradeProcessor_, &UpgradeProcessor::installProgress, this, &Launcher::onUpgradeProgress);
    connect(upgradeProcessor_, &UpgradeProcessor::installFinished, this, &Launcher::onInstallFinished);
    connect(upgradeProcessor_, &UpgradeProcessor::upgradeCanceled, this, &Launcher::onUpgradeCanceled);
    connect(upgradeProcessor_, &UpgradeProcessor::startUpgrade, this, &Launcher::startUpgrade);
    connect(upgradeProcessor_, &UpgradeProcessor::downloadError, this, &Launcher::onDownloadError);
    connect(this, &Launcher::upgradeModules, upgradeProcessor_, &UpgradeProcessor::upgradeModules);
    connect(this, &Launcher::stopDownloadModules, upgradeProcessor_, &UpgradeProcessor::stopDownloadModules);
    processThread_->start();
    QTimer::singleShot(100, upgradeProcessor_, &UpgradeProcessor::startProgress);
}

void Launcher::startMainFrame()
{
    Config::clearUpgradeSelection();

    auto mainframeVersion = Config::mainframeInfo();
#ifdef DEBUG
    QProcess::startDetached("../../mainFrame/" + mainframeVersion + "/uTools_hmd.exe", QStringList(), "../../mainFrame/" + mainframeVersion);
#else
    QProcess::startDetached("../../mainFrame/" + mainframeVersion + "/uTools_hm.exe",  QStringList(), "../../mainFrame/" + mainframeVersion);
#endif
    exit(0);
}

void Launcher::startUpgrade(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &modules)
{
    ui->ProgressBar->setValue(0);
    ui->ProgressBar->show();
    status_ = tr("downloading %1%");
    ui->StatusLabel->setText(status_.arg(0));
    ui->StatusLabel->show();
    ui->CancelBtn->show();

    emit upgradeModules(remoteGrpVersion, modules);
}

void Launcher::onGroupUpgradableQueryReply(int httpStatusCode, int networkError, QJsonObject remoteGrpInfo)
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
    auto remoteGrpVersion = remoteGrpInfo.value(Upgrade::KEY_VERSION).toString();
    if (remoteGrpVersion.isNull() || remoteGrpVersion.isEmpty()) {
        LOG(INFO) << "invalid remote group version";
        startMainFrame();
        return;
    }
    if (Upgrade::versionCmp(Settings::ignoreGroupVersion(), remoteGrpVersion) == 0) {
        LOG(INFO) << "remote version " << remoteGrpVersion.toStdString() << " ignored";
        startMainFrame();
        return;
    }

    QMap<QString, QJsonObject> modulesToUpgrade;
    auto modulesArray = remoteGrpInfo.value(Upgrade::KEY_MODULES).toArray();
    for (auto modulesValue : modulesArray) {
        auto modulesObj = modulesValue.toObject();
        if (!modulesObj.isEmpty()) {
            auto moduleName = modulesObj.value(Upgrade::KEY_MODULENAME).toString();
            if (modulesInfo_.contains(moduleName)) {
                if (Upgrade::versionCmp(modulesInfo_.value(moduleName), modulesObj.value(Upgrade::KEY_VERSIONNAME).toString()) == -1) {
                    modulesToUpgrade.insert(moduleName, modulesObj);
                }
            } else {
                modulesToUpgrade.insert(moduleName, modulesObj);
            }
        }
    }

    if (Upgrade::versionCmp(Config::groupVersion(), remoteGrpVersion) == 0 && modulesToUpgrade.count() == 0) {
        LOG(INFO) << "remote version " << remoteGrpVersion.toStdString() << " is as same as local version";
        startMainFrame();
        return;
    }

//    auto forced = remoteGrpInfo.value(Upgrade::KEY_FORCED).toBool();
//    if (forced) {
//        startUpgrade(remoteGrpVersion, modulesToUpgrade);
//    } else {
        CommonDialog *dialog = new CommonDialog("", CommonDialog::NoButton, this);
        auto updateLog = remoteGrpInfo.value(Upgrade::KEY_UPDATELOG).toString();
        auto upgradableModulesSelector = new UpgradableModulesSelector(remoteGrpVersion, updateLog, modulesToUpgrade, this);
        connect(upgradableModulesSelector, &UpgradableModulesSelector::upgrade, dialog, &CommonDialog::close);
        connect(upgradableModulesSelector, &UpgradableModulesSelector::ignored, dialog, &CommonDialog::close);
        connect(upgradableModulesSelector, &UpgradableModulesSelector::upgrade, this, &Launcher::startUpgrade);
        connect(upgradableModulesSelector, &UpgradableModulesSelector::ignored, this, &Launcher::startMainFrame);
        connect(dialog, &CommonDialog::sigClosed, this, &Launcher::startMainFrame);
        dialog->setDisplayWidget(upgradableModulesSelector);
        dialog->exec();
//    }
}

void Launcher::onUpgradeProgress(int progress)
{
    if (progress < 0) {
        progress = 0;
    } else if (progress > 100) {
        progress = 100;
    }
    ui->StatusLabel->setText(status_.arg(progress));
    ui->ProgressBar->setValue(progress);
}

void Launcher::onDownloadFinished()
{
    ui->ProgressBar->setValue(0);
    ui->ProgressBar->show();
    status_ = tr("installing %1%");
    ui->StatusLabel->setText(status_.arg(0));
    ui->StatusLabel->show();
    ui->CancelBtn->hide(); //安装时无法取消
}

void Launcher::onInstallFinished()
{
    ui->StatusLabel->setText(tr("upgrade finished"));
    startMainFrame();
}

void Launcher::onUpgradeCanceled()
{
    ui->StatusLabel->setText(tr("cancel upgrading..."));
    startMainFrame();
}

void Launcher::loadStyleSheet()
{
    QFile styleSheetFile(":/res/qss/launcher.qss");
    if (styleSheetFile.open(QFile::ReadOnly)) {
        auto styleSheet = styleSheetFile.readAll();
        setStyleSheet(styleSheet);
        styleSheetFile.close();
    } else {
        LOG(ERROR) << "open :/res/qss/launcher.qss failed";
    }
}

void Launcher::onCancelUpgradeClicked()
{
    LOG(INFO) << "onCancelUpgradeClicked" << downloadFailed_;
    if(downloadFailed_) {
        downloadFailed_ = false;
        startMainFrame();
        return;
    }

    upgradeProcessor_->cancelClicked();

    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, this);
    auto cancelUpgradeConfirmDlg = new CancelUpgradeConfirmDlg;
    dialog->setDisplayWidget(cancelUpgradeConfirmDlg);
    connect(dialog, &CommonDialog::sigAccepted, this, [this](bool accept) {
        if (accept) {
            if (!upgradeProcessor_->isInstalling()) {
                LOG(INFO) << "stop download";
                emit stopDownloadModules();
            } else {
                LOG(INFO) << "request cancel install";
                upgradeProcessor_->requestCancelInstall();
            }
        } else {
            upgradeProcessor_->cancelCanceled();
        }
    });
    dialog->exec();
}

void Launcher::onDownloadError()
{
    ui->StatusLabel->setText(tr("Download failed"));
    ui->StatusLabel->setStyleSheet("color: #FF2F6F;");
    ui->StatusLabel->style()->unpolish(ui->StatusLabel);
    ui->StatusLabel->style()->polish(ui->StatusLabel);
    downloadFailed_ = true;
}
