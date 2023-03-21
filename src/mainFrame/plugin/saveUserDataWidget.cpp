#include "saveUserDataWidget.h"
#include "ui_saveUserDataWidget.h"

SaveUserDataWidget::SaveUserDataWidget(const QString &warning, bool saveUserData, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SaveUserDataWidget)
{
    ui->setupUi(this);

    ui->saveUserData->setChecked(saveUserData);

    ui->warningMsg->setText(warning);
    connect(ui->ok,&QPushButton::clicked,[this]{
        emit sigAccepted(true, ui->saveUserData->isChecked());
        close();
    });
    connect(ui->cancel,&QPushButton::clicked,[this]{
       emit sigAccepted(false, ui->saveUserData->isChecked());
       close();
    });

    this->setStyleSheet("color:#FFFFFF");
    ui->warningPic->setPixmap(QPixmap(":/res/images/ic_tips_warning.svg"));
    ui->cancel->setStyleSheet("background-color:#5B6479;border-radius:3px");
    ui->ok->setStyleSheet("background-color:#00A5FF;border-radius:3px");
    ui->saveUserData->setStyleSheet("QCheckBox::indicator:enabled:unchecked {image: url(:/res/images/ic_checkbox.svg);}"
                            "QCheckBox::indicator:enabled:unchecked:hover {image: url(:/res/images/ic_checkbox_mouseover.svg);}"
                            "QCheckBox::indicator:enabled:checked {image: url(:/res/images/ic_checkbox_p.svg);}");
}

SaveUserDataWidget::~SaveUserDataWidget()
{
    delete ui;
}

void SaveUserDataWidget::hideSaveUserData()
{
    ui->saveUserData->setEnabled(false);
    ui->saveUserData->setVisible(false);
}
