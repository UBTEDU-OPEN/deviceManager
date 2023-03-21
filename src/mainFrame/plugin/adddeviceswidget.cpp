#include "adddeviceswidget.h"
#include "ui_adddeviceswidget.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileInfo>
#include <QDialog>
#include <QHBoxLayout>
#include <QRegularExpression>

#include "excelHelper.h"
#include "commondialog.h"
#include "warningwidget.h"

AddDevicesWidget::AddDevicesWidget(int maxNum, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddDevicesWidget),
    selectedId_(0),
    maxNum_(maxNum)
{
    ui->setupUi(this);
    QButtonGroup* group = new QButtonGroup(this);
    group->addButton(ui->inputRadio,0);
    group->addButton(ui->importRadio,1);
    ui->inputRadio->setChecked(true);
    ui->choose->setEnabled(false);
    connect(group,QOverload<int>::of(&QButtonGroup::buttonClicked),this,&AddDevicesWidget::onRadioBtnClicked);
    connect(ui->choose,&QPushButton::clicked,this,&AddDevicesWidget::onChoose);
    connect(ui->preview,&QPushButton::clicked,this,&AddDevicesWidget::previewSn2Id);
    connect(ui->sn,&QLineEdit::textChanged,this,&AddDevicesWidget::onTextChanged);
    connect(ui->sn,&QLineEdit::editingFinished,this,&AddDevicesWidget::onEditingFinished);
    ui->preview->hide();

    QFile styleSheet(":/res/qss/addDevicesWidget.qss");
    if(styleSheet.open(QIODevice::ReadOnly))
    {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }

    QRegExp regx("^[a-zA-Z0-9]+$");
    QValidator *validator = new QRegExpValidator(regx,ui->sn);
    ui->sn->setValidator(validator);
}

AddDevicesWidget::~AddDevicesWidget()
{
    delete ui;
}

void AddDevicesWidget::previewSn2Id()
{
    //    QDialog *dialog = new QDialog(this);
    //    dialog->setLayout(new QHBoxLayout);
    //    dialog->setModal(true);
    //    dialog->setAttribute(Qt::WA_DeleteOnClose);
        CommonDialog *dialog = new CommonDialog(tr("ubtech's AiBox"), CommonDialog::NoButton, this);
        dialog->setMinimumSize(700,550);
        dialog->setMaximumSize(700,550);
        QTableWidget* table = new QTableWidget(sns2ids_.size(),2,dialog);
        QStringList headerList;
        headerList << tr("SN") << tr("NAME");
        table->setHorizontalHeaderLabels(headerList);
        table->verticalHeader()->hide();
        int row = 0;
        for(auto sn2id : sns2ids_) {
            table->setItem(row, 0, new QTableWidgetItem(sn2id.first));
            table->setItem(row, 1, new QTableWidgetItem(sn2id.second));
            table->setRowHeight(row, 40);
            row++;
        }
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        table->setShowGrid(false);
        table->horizontalHeader()->setStyleSheet("QHeaderView::section{border:none;background-color:#313747;color:#FFFFFF}");
        table->setColumnWidth(0,430);
        table->setColumnWidth(1,210);
        table->setAlternatingRowColors(true);
        table->setStyleSheet("QTableWidget{border-radius: 4px;color: #FFFFFF;background-color: #2B3140;alternate-background-color: #313747;}"
                             "QScrollBar:vertical{" //垂直滑块整体
                             "background-color:#242A37;"  //背景色
                             "width:12px;"
                             "padding-top:25px;"    //上预留位置（放置向上箭头）
                             "padding-bottom:0px;" //下预留位置（放置向下箭头）
                             "padding-left:4.5px;"    //左预留位置（美观）
                             "padding-right:4.5px;"   //右预留位置（美观）
                             "image: url(:/system/A-drop-down-box.png)}"

                             "QScrollBar::handle:vertical{"//滑块样式
                             "background:#777F94;"  //滑块颜色
                             "border-radius:1.5px;"   //边角圆润
                             "min-height:80px;" //滑块最小高度
                             "}"

                             "QScrollBar::add-page:vertical{background-color:#242A37}"
                             "QScrollBar::sub-page:vertical{background-color:#242A37}"
                             "QScrollBar::add-line:vertical{"//向下箭头样式
                             "border:none; background:none}"
                             "QScrollBar::sub-line:vertical{"//向上箭头样式
                             "border:none; background:#313747; height:25px}");


    //    dialog->layout()->addWidget(table);
    //    dialog->show();


        dialog->setDisplayWidget(table);
        dialog->show();
}

void AddDevicesWidget::onRadioBtnClicked(int id)
{
    selectedId_ = id;
    if (id == 0) {
        ui->choose->setEnabled(false);
        ui->importIcon->setPixmap(QPixmap(":/res/images/ic_folder_disable.png"));
        ui->sn->setEnabled(true);
        if(!ui->sn->text().isEmpty()) {
            emit setOkBtnEnabled(true);
        } else {
            emit setOkBtnEnabled(false);
        }
    } else {
        ui->choose->setEnabled(true);
        ui->importIcon->setPixmap(QPixmap(":/res/images/ic_folder.png"));
        ui->sn->setEnabled(false);
        if (!sns2ids_.empty()) {
            emit setOkBtnEnabled(true);
        } else {
            emit setOkBtnEnabled(false);
        }
    }
}

void AddDevicesWidget::onChoose()
{
    auto filePath = QFileDialog::getOpenFileName(this, tr("imoport sn"),".",tr("Excel (*.xlsx *.xls)"));
    if (!filePath.isEmpty()) {
        sns2ids_ = ExcelHelper::readSnAndIdFromExcel(filePath);
        auto SNIsValid = [](const QString &sn) -> bool {
            return !sn.isEmpty() && sn.count() >= 17 && !sn.contains(QRegularExpression("\\W"));
        };
        QList<QPair<QString, QString>> validSNs2IDs;
        for (auto sn2id : sns2ids_) {
            if (SNIsValid(sn2id.first)) {
                validSNs2IDs.append(sn2id);
            }
        }
        sns2ids_ = validSNs2IDs;
        if (sns2ids_.size() > maxNum_) {
            warningwidget *warning = new warningwidget(tr("import failed, current device num:%1, Maximum allowable import quantity: %2.").arg(sns2ids_.size()).arg(maxNum_));
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton);
            dialog->setMinimumSize(580,170);
            dialog->setDisplayWidget(warning);
            dialog->show();
        } else if (!sns2ids_.empty()) {
            ui->importIcon->setPixmap(QPixmap(":/res/images/ic_excel.svg"));
            QFileInfo fileInfo(filePath);
            ui->fileName->setText(fileInfo.fileName());
            ui->choose->setText(QString::fromLocal8Bit("重新选择"));
            ui->preview->show();
            emit setOkBtnEnabled(true);
        }
    }
}

void AddDevicesWidget::onAccepted(bool accepted)
{
    if (accepted) {
        QList<QPair<QString,QString>> devices;
        if (selectedId_ == 0) {
            QString sn = ui->sn->text();
            if (sn.size() < 17 || sn.contains(QRegularExpression("\\W"))) {
                CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OnlyOkButton, this);
                QString upgradeWarningMsg = tr("invalid sn format, check and reenter please");
                warningwidget *warning = new warningwidget(upgradeWarningMsg,dialog);
                dialog->setDisplayWidget(warning);
                dialog->exec();
                return;
            } else {
                QPair<QString, QString> sn2id = {sn, ""};
                devices.append(sn2id);
            }
        } else {
            if (!sns2ids_.empty()) {
                devices = sns2ids_;
            }
        }
        if (!devices.empty()) {
            emit addDevices(devices);
        }
    }
}

void AddDevicesWidget::onTextChanged()
{
    if (!ui->sn->text().isEmpty()) {
        emit setOkBtnEnabled(true);
    } else {
        emit setOkBtnEnabled(false);
    }
}

void AddDevicesWidget::onEditingFinished()
{
    if (!ui->sn->text().isEmpty()) {
        emit setFocusToOkBtn();
    }
}
