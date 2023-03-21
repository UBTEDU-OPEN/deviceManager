#include "platformselectiondialog.h"
#include "ui_platformselectiondialog.h"

#include "devicePluginInterface.h"
#include "pluginManager.h"
#include "settings.h"
#include "config.h"
#include "logHelper.h"

#include <QDir>
#include <QPainter>

PlatformSelectionDialog::PlatformSelectionDialog(const PluginManager *pluginManager, QWidget *parent)
    : QDialog(parent)
    , pluginManager_(pluginManager)
    , ui(new Ui::PlatformSelectionDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::FramelessWindowHint|Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    closeBtn_ = new QPushButton(this);
    closeBtn_->setObjectName("close");
    closeBtn_->setFixedSize(30,30);

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

    for (auto plugin : pluginManager->plugins) {
        auto layout = new QVBoxLayout;
        auto btn = new QPushButton;
        btn->setFixedSize(120, 120);
        auto dir = QDir(Settings::pluginsAbsPath());
        auto pluginsInfo = Config::pluginsInfo();
        if (!dir.cd(plugin->name() + "/" + pluginsInfo[plugin->name()])) {
            LOG(INFO) << "no plugin named " << plugin->name().toStdString();
            continue;
        }
        QString btnStylesheetStr;
        if (!dir.cd(Settings::VALUE_PLUGIN_RES_FOLDER_NAME)) {
            LOG(INFO) << "find no resource(res) folder in plugin folder for " << plugin->name().toStdString();
            btnStylesheetStr = "QPushButton:enabled{border-image:url(:/res/images/ic_aibox.png);}"
                               "QPushButton:hover{border-image:url(:/res/images/ic_aibox.png);}"
                               "QPushButton:pressed{border-image:url(:/res/images/ic_aibox_p.png);}"
                               "QPushButton:disabled{border-image:url(:/res/images/ic_aibox_disable.png);}";
        } else {
            auto imagesUrls = plugin->deviceTypeSelectionImagesUrls();
            for (int i = 0; i < DevPluginInterface::ImageType::CountNum; ++i) {
                btnStylesheetStr += "QPushButton:" + imgeType2qssState(i) + "{border-image:url(" + dir.absoluteFilePath(imagesUrls[i]) + ");}";
            }
        }
        btn->setStyleSheet(btnStylesheetStr);
        btn->setProperty("selection", plugin->name());
        layout->addWidget(btn);
        auto label = new QLabel(plugin->name());
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("QLabel{color:white;}");
        layout->addWidget(label);
        ui->SelectionLayout->insertLayout(0, layout);

        connect(btn, &QPushButton::clicked, this, &PlatformSelectionDialog::onSelection);
    }

    connect(closeBtn_, &QPushButton::clicked, this, &PlatformSelectionDialog::onCloseClicked);

//    if(parent != nullptr) {
//        setFixedSize(parent->size());
//    }
}

PlatformSelectionDialog::~PlatformSelectionDialog()
{
    delete ui;
}

void PlatformSelectionDialog::onSelection()
{
    if (auto btn = dynamic_cast<QPushButton*>(sender())) {
        emit selection(btn->property("selection").toString());
    }
    close();
}

void PlatformSelectionDialog::onCloseClicked()
{
    close();
}

//void PlatformSelectionDialog::paintEvent(QPaintEvent *event)
//{
//    QPainter painter(this);
//    painter.setRenderHint(QPainter::Antialiasing);
//    painter.setBrush(QBrush(QColor(30,35,45, 178)));
//    painter.setPen(Qt::NoPen);
//    painter.drawRect(rect());
//}

void PlatformSelectionDialog::resizeEvent(QResizeEvent *event)
{
    auto pos = ui->centralWidget->geometry().topRight();
    auto x = pos.x() - closeBtn_->width()/2;
    auto y = pos.y() - closeBtn_->height()/2;
    closeBtn_->move(x,y);
}
