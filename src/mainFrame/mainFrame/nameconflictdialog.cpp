#include "nameconflictdialog.h"
#include "ui_nameconflictdialog.h"

#include <QFile>

NameConflictDialog::NameConflictDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NameConflictDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint|Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);

    connect(ui->close,&QPushButton::clicked,this,&NameConflictDialog::close);
    connect(ui->ok,&QPushButton::clicked,this,&NameConflictDialog::close);

    QFile styleSheet(":/res/qss/nameConflictDialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
}

NameConflictDialog::~NameConflictDialog()
{
    delete ui;
}

void NameConflictDialog::setWarningMsg(const QString& msg)
{
    ui->warningMsg->setText(msg);
    update();
}
