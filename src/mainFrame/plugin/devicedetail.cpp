#include "devicedetail.h"
#include "ui_devicedetail.h"

#include "devicePluginInterface.h"
#include "settings.h"
#include "config.h"

#include <QDebug>
#include <iostream>
#include <QRegExp>
#include <QRegExpValidator>
#include <QDir>

DeviceDetail::DeviceDetail(const DevPluginInterface *plugin,
        const QString& sn,
        const QString& name,
        const QString& version,
        int power,
        const QString& powerImage,
        bool connected,
        QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DeviceDetail)
    , plugin_(plugin)
    , editing_(false)
{
    ui->setupUi(this);
    name_ = name;
    setAttribute(Qt::WA_DeleteOnClose);
    ui->sn->setText(QString::fromLocal8Bit(" %1").arg(sn));
    ui->name->setText(name);
    ui->name->setStyleSheet(" background-color:transparent; margin:0px 0px;padding: 0px 0px");
    ui->version->setText(QString::fromLocal8Bit(" %1").arg(version));
    ui->name->setEnabled(editing_);

    auto imgeType2qssState = [](int imageType) -> QString {
        if (imageType == static_cast<int>(DevPluginInterface::Normal)) {
            return "enabled";
        } else if (imageType == static_cast<int>(DevPluginInterface::Hover)) {
            return "hover";
        } else if (imageType == static_cast<int>(DevPluginInterface::Pressed)) {
            return "pressed";
        } else if (imageType == static_cast<int>(DevPluginInterface::Disable)) {
            return "disabled";
        } else {
            return "enabled";
        }
    };
    QString labelStylesheetStr;
    auto dir = QDir(Settings::pluginsAbsPath());
    auto pluginsInfo = Config::pluginsInfo();
    if (!dir.cd(plugin->name() + "/" + pluginsInfo[plugin->name()]) || !dir.cd(Settings::VALUE_PLUGIN_RES_FOLDER_NAME)) {
        labelStylesheetStr = "QLabel:enabled{border-image:url(:/res/images/ic_yanshee.png);}"
                             "QLabel:hover{border-image:url(:/res/images/ic_yanshee.png);}"
                             "QLabel:pressed{border-image:url(:/res/images/ic_yanshee_p.png);}"
                             "QLabel:disabled{border-image:url(:/res/images/ic_yanshee_disable.png);}";
    } else {
        auto imagesUrls = plugin->deviceTypeSelectionImagesUrls();
        for (int i = 0; i < DevPluginInterface::ImageType::CountNum; ++i) {
            labelStylesheetStr += "QLabel:" + imgeType2qssState(i) + "{border-image:url(" + dir.absoluteFilePath(imagesUrls[i]) + ");}";
        }
    }
    ui->computer->setStyleSheet(labelStylesheetStr);
    ui->computer->setEnabled(connected);

    if (connected) {
        ui->power->setText(QString("%1%").arg(power));
        ui->powerIcon->setPixmap(QPixmap(powerImage));
        ui->connect->setText(tr("online"));
//        ui->computer->setStyleSheet("border-image:url(:/res/images/ic_aibox.png)");
        ui->lbicconnected->setPixmap(QPixmap(":/res/images/ic_online.svg"));
    } else {
        ui->power->hide();
        ui->powerIcon->hide();
        ui->connect->setText(tr("offline"));
//        ui->computer->setStyleSheet("border-image:url(:/res/images/ic_aibox_disable.png)");
        ui->lbicconnected->setPixmap(QPixmap(":/res/images/ic_offline.svg"));
    }

    ui->edit->setStyleSheet("border-image:url(:/res/images/ic.edit.svg)");
    connect(ui->edit, &QPushButton::clicked, [this] {
        editing_ = !editing_;
        ui->name->setEnabled(editing_);
        if (editing_) {ui->name->selectAll();}
        else {ui->name->deselect();}
    });
    connect(ui->name, &QLineEdit::editingFinished, [this]() {
        QString newName = ui->name->text();
        emit deviceNameEdited(newName);
        editing_ = false;
        ui->name->setEnabled(editing_);
    });

    QRegExp regx("^[a-zA-Z0-9_\u4e00-\u9fa5]+$");
    QValidator *validator = new QRegExpValidator(regx,ui->name);
    ui->name->setValidator(validator);
    connect(ui->name, &QLineEdit::textChanged, [this](QString s){
        s.replace(QString::fromLocal8Bit("【"), "");
        s.replace(QString::fromLocal8Bit("】"), "");
        s.replace(QString::fromLocal8Bit("？"), "");
        s.replace(QString::fromLocal8Bit("！"), "");
        s.replace(QString::fromLocal8Bit("·"), "");
        s.replace(QString::fromLocal8Bit("￥"), "");
        s.replace(QString::fromLocal8Bit("……"), "");
        s.replace(QString::fromLocal8Bit("（"), "");
        s.replace(QString::fromLocal8Bit("）"), "");
        s.replace(QString::fromLocal8Bit("——"), "");
        s.replace(QString::fromLocal8Bit("、"), "");
        s.replace(QString::fromLocal8Bit("："), "");
        s.replace(QString::fromLocal8Bit("；"), "");
        s.replace(QString::fromLocal8Bit("“"), "");
        s.replace(QString::fromLocal8Bit("”"), "");
        s.replace(QString::fromLocal8Bit("’"), "");
        s.replace(QString::fromLocal8Bit("‘"), "");
        s.replace(QString::fromLocal8Bit("《"), "");
        s.replace(QString::fromLocal8Bit("》"), "");
        s.replace(QString::fromLocal8Bit("，"), "");
        s.replace(QString::fromLocal8Bit("。"), "");
        ui->name->setText(s);
//        qDebug() << "payne:" << s.size();
    });
    ui->name->setMaxLength(32);
}

DeviceDetail::~DeviceDetail()
{
    qDebug() << ui->name << "deleted.";
    delete ui;
}
