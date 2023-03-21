#include "closewarningdialog.h"
#include "ui_closewarningdialog.h"

#include <QFile>

CloseWarningDialog::CloseWarningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CloseWarningDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint|Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    connect(ui->cancel,&QPushButton::clicked,[this]{
        emit sigAccepted(false);
        close();
    });
    connect(ui->ok,&QPushButton::clicked,[this]{
        emit sigAccepted(true);
        close();
    });
    connect(ui->close,&QPushButton::clicked,[this]{
        emit sigAccepted(false);
        close();
    });
    QFile styleSheet(":/res/qss/closeWarningDialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
}

CloseWarningDialog::~CloseWarningDialog()
{
    delete ui;
}
