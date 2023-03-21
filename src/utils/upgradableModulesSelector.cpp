#include "upgradableModulesSelector.h"
#include "ui_upgradableModulesSelector.h"

#include "settings.h"
#include "upgrade.h"
#include "logHelper.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonValue>
#include <QTableWidgetItem>

UpgradableModulesSelector::UpgradableModulesSelector(const QString &remoteGrpVersion,
                                                     const QString &updateLog,
                                                     const QMap<QString, QJsonObject> &upgradeModules,
                                                     QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::UpgradableModulesSelector)
  , remoteGrpVersion_(remoteGrpVersion)
{
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);
    loadStyleSheet();

    ui->UpdateLog->setText(updateLog);
    initModulesTable(upgradeModules);

    connect(ui->UpgradeBtn, &QPushButton::clicked, this, &UpgradableModulesSelector::onUpgradeClicked);
    connect(ui->IgnoreBtn, &QPushButton::clicked, this, &UpgradableModulesSelector::onIgnoreClicked);
}

UpgradableModulesSelector::~UpgradableModulesSelector()
{
    delete ui;
}

void UpgradableModulesSelector::onIgnoreClicked()
{
    Settings::saveIgnoreGroupVersion(remoteGrpVersion_);
    emit ignored(remoteGrpVersion_);
    close();
}

void UpgradableModulesSelector::onUpgradeClicked()
{
    QMap<QString, QJsonObject> upgradeModules;
    for (int row = 0; row < ui->UpgradableModulesTable->rowCount(); ++row) {
        if (ui->UpgradableModulesTable->item(row, 0)->checkState() == Qt::Checked) {
            upgradeModules.insert(ui->UpgradableModulesTable->item(row, 0)->data(Qt::UserRole + 2).toString(), ui->UpgradableModulesTable->item(row, 0)->data(Qt::UserRole + 1).toJsonObject());
        }
    }
    if(upgradeModules.empty()) {
        return;
    }
    emit upgrade(remoteGrpVersion_, upgradeModules);
    close();
}

void UpgradableModulesSelector::loadStyleSheet()
{
    QFile styleSheetFile(":/res/qss/upgradableModulesSelector.qss");
    if (styleSheetFile.open(QFile::ReadOnly)) {
        auto styleSheetStr = styleSheetFile.readAll();
        setStyleSheet(styleSheetStr);
        styleSheetFile.close();
    } else {
        LOG(ERROR) << "open :/res/qss/upgradableModulesSelector.qss failed";
    }
}

void UpgradableModulesSelector::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    auto w = ui->UpgradableModulesTable->width() / 3;
    ui->UpgradableModulesTable->horizontalHeader()->resizeSection(0, w);
    ui->UpgradableModulesTable->horizontalHeader()->resizeSection(1, w);
    ui->UpgradableModulesTable->horizontalHeader()->resizeSection(2, w);
}

void UpgradableModulesSelector::initModulesTable(const QMap<QString, QJsonObject> &upgradeModules)
{
    ui->UpgradableModulesTable->setShowGrid(false);
    ui->UpgradableModulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->UpgradableModulesTable->setFocusPolicy(Qt::NoFocus);
    ui->UpgradableModulesTable->setSelectionMode(QAbstractItemView::NoSelection);
    ui->UpgradableModulesTable->setColumnCount(3);
    ui->UpgradableModulesTable->setRowCount(upgradeModules.count());
    QStringList horizontalHeaderLabels = {
        tr("     available module"),
        tr("version"),
        tr("size")
    };
    ui->UpgradableModulesTable->setHorizontalHeaderLabels(horizontalHeaderLabels);
    ui->UpgradableModulesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui->UpgradableModulesTable->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->UpgradableModulesTable->horizontalHeader()->setSelectionMode(QAbstractItemView::NoSelection);
    int row = 0;
    for (auto modulesObj : upgradeModules) {
        if (!modulesObj.isEmpty()) {
            auto moduleName = modulesObj.value(Upgrade::KEY_MODULENAME).toString();
            auto displayName = convertModuleName(moduleName);
            auto item = new QTableWidgetItem(displayName);
            item->setData(Qt::UserRole + 1, modulesObj);
            item->setData(Qt::UserRole + 2, moduleName);
            item->setCheckState(Qt::Checked);
            //item->setTextAlignment(Qt::AlignLeft);
            ui->UpgradableModulesTable->setItem(row, 0, item);
            auto moduleVersion = modulesObj.value(Upgrade::KEY_VERSIONNAME).toString();
            auto displayVersion = convertVersion(moduleVersion);
            item = new QTableWidgetItem(displayVersion);
            item->setData(Qt::UserRole + 1,moduleVersion);
            //item->setTextAlignment(Qt::AlignLeft);
            ui->UpgradableModulesTable->setItem(row, 1, item);
            auto size = modulesObj.value(Upgrade::KEY_PACKAGESIZE).toDouble();
            double mSize = size / (1024*1024);
            if(mSize < 0.11) {
                mSize = 0.11;
            }
            item = new QTableWidgetItem(QString::number(mSize,'f',1)+"MB");
            item->setData(Qt::UserRole + 1,size);
            //item->setTextAlignment(Qt::AlignLeft);
            ui->UpgradableModulesTable->setItem(row, 2, item);
            ++row;
        }
    }
}

QString UpgradableModulesSelector::convertModuleName(const QString& name)
{
    if(0 == name.compare("mainFrame",Qt::CaseInsensitive)) {
        return tr("uTools Main");
    } else if(0 == name.compare("launcher",Qt::CaseInsensitive)) {
        return tr("uTools Updater");
    } else if(0 == name.compare("aibox",Qt::CaseInsensitive)) {
        return tr("AIbox Console");
    } else if(0 == name.compare("yanshee",Qt::CaseInsensitive)) {
        return tr("Yanshee Console");
    }
    return "";
}
QString UpgradableModulesSelector::convertVersion(const QString& version)
{
    if(3 == version.count('.')) {
        auto pos = version.lastIndexOf('.');
        auto subStr = "V" + version.mid(0,pos);
        return subStr;
    }
    return version;
}
