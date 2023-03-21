#include "warningwidget.h"
#include "ui_warningwidget.h"

warningwidget::warningwidget(QString text, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::warningwidget)
{
    ui->setupUi(this);
    ui->lbwarning->setText(text);
}

warningwidget::~warningwidget()
{
    delete ui;
}
