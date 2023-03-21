#include "renamewidget.h"
#include "ui_renamewidget.h"

#include <QFile>
#include <QRegExp>
#include <QRegExpValidator>

renamewidget::renamewidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::renamewidget)
{
    ui->setupUi(this);

    setFixedSize(400,120);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    QFile styleSheet(":/res/qss/renamewidget.qss");
    if(styleSheet.open(QIODevice::ReadOnly))
    {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }



    connect(ui->pbclose, &QPushButton::clicked, this, &renamewidget::close);
    connect(ui->pbcancel, &QPushButton::clicked, this, &renamewidget::close);
    connect(ui->pbok, &QPushButton::clicked, this, &renamewidget::onRename);

    QRegExp regx("^[a-zA-Z0-9_\u4e00-\u9fa5]+$");
    QValidator *validator = new QRegExpValidator(regx,ui->lineEdit);
    ui->lineEdit->setValidator(validator);
    connect(ui->lineEdit, &QLineEdit::textChanged, [this](QString s){
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
        ui->lineEdit->setText(s);
    });
    ui->lineEdit->setMaxLength(16);
}

renamewidget::~renamewidget()
{
    delete ui;
}

void renamewidget::onRename()
{
    if(ui->lineEdit->text() != "")
    {
        emit sigRename(ui->lineEdit->text());
        this->close();
    }
}
