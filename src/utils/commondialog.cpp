#include "commondialog.h"
#include "ui_commondialog.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QMouseEvent>
#include <QFile>

#include "logHelper.h"

CommonDialog::CommonDialog(const QString& title, ButtonType type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CommonDialog)
    , leftButtonPressed_(false)
    , closeOnOk_(true)
    , closeOnClose_(true)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);

    ui->titleName->setText(title);
    switch (type) {
    case ButtonType::NoButton:
        ui->verticalLayout->removeItem(ui->buttonHLayout);
        ui->verticalLayout->removeItem(ui->horizontalLayout);
        ui->buttonHLayout->deleteLater();
        ui->horizontalLayout->deleteLater();
        hideBtn(ui->okButton);
        hideBtn(ui->cancelButton);
        hideBtn(ui->ok2);
        break;
    case ButtonType::OnlyOkButton:
        ui->verticalLayout->removeItem(ui->buttonHLayout);
        ui->buttonHLayout->deleteLater();
        hideBtn(ui->okButton);
        hideBtn(ui->cancelButton);
        break;
    case ButtonType::OkCancelButton:
        ui->verticalLayout->removeItem(ui->horizontalLayout);
        ui->horizontalLayout->deleteLater();
        hideBtn(ui->ok2);
        break;
    default:
        break;
    }
    connect(ui->closeButton,&QPushButton::clicked,this,&CommonDialog::onCloseClicked);
    connect(ui->okButton,&QPushButton::clicked,this,&CommonDialog::onOkClicked);
    connect(ui->cancelButton,&QPushButton::clicked,this,&CommonDialog::onCancelClicked);
    connect(ui->ok2,&QPushButton::clicked,this,&CommonDialog::onOk2Clicked);

    QFile styleSheet(":/res/qss/commonDialog.qss");
    if(styleSheet.open(QIODevice::ReadOnly))
    {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
}

CommonDialog::~CommonDialog()
{
    delete ui;
}

void CommonDialog::setDisplayWidget(QWidget* widget)
{
    ui->text->hide();
    QHBoxLayout* layout = new QHBoxLayout(ui->content);
    layout->addWidget(widget);
    adjustSize();
}

void CommonDialog::setDisplayText(const QString& text)
{
    ui->content->hide();
    ui->text->setText(text);
    adjustSize();
}

void CommonDialog::mousePressEvent(QMouseEvent *event)
{
    if ( event->button() == Qt::LeftButton &&
         ui->title->rect().contains(event->pos()) ) {
        lastPos_ = event->globalPos();
        leftButtonPressed_ = true;
    }
    QDialog::mousePressEvent(event);
}

void CommonDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->button() == Qt::LeftButton && leftButtonPressed_) {
        leftButtonPressed_ = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void CommonDialog::mouseMoveEvent(QMouseEvent *event)
{
    if ( event->buttons().testFlag(Qt::LeftButton) &&  leftButtonPressed_ ) {
        QPoint position = pos() + event->globalPos() - lastPos_;
        move(position.x(), position.y());
        lastPos_ = event->globalPos();
    }
    QDialog::mouseMoveEvent(event);
}
void CommonDialog::setOkBtnText(const QString& text)
{
    ui->okButton->setText(text);
}
void CommonDialog::setCancelBtnText(const QString& text)
{
    ui->cancelButton->setText(text);
}
void CommonDialog::setOk2BtnText(const QString& text)
{
    ui->ok2->setText(text);
}

void CommonDialog::setCloseOnOk(bool closeOnOk)
{
    closeOnOk_ = closeOnOk;
}

void CommonDialog::setCloseOnClose(bool closeOnClose)
{
    closeOnClose_ = closeOnClose;
}

void CommonDialog::onOkClicked()
{
    emit sigAccepted(true);
    if (closeOnOk_) {
        close();
    }
}
void CommonDialog::onCancelClicked()
{
    emit sigAccepted(false);
    close();
}
void CommonDialog::onOk2Clicked()
{
    emit sigAccepted(true);
    close();
}
void CommonDialog::onCloseClicked()
{
    emit sigClosed();
    if(closeOnClose_) {
        close();
    }
}
void CommonDialog::hideBtn(QPushButton* btn)
{
    btn->setEnabled(false);
    btn->hide();
}
void CommonDialog::onSetOkBtnEnabled(bool enabled)
{
    ui->okButton->setEnabled(enabled);
}

void CommonDialog::onSetFocusToOkBtn()
{
    ui->okButton->setFocus();
}

void CommonDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Escape) {
        LOG(INFO) << "CommonDialog::keyPressEvent";
        e->accept();
        return;
    }
    return QDialog::keyPressEvent(e);
}

