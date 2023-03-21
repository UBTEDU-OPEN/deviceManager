#include "backgroundwidget.h"
#include "ui_backgroundwiget.h"

#include <QPixmap>
#include <QDebug>

BackgroundWidget::BackgroundWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BackgroundWiget)
{
    ui->setupUi(this);
    connect(ui->add,&QPushButton::clicked,this,&BackgroundWidget::addBtnClicked);
}

BackgroundWidget::~BackgroundWidget()
{
    delete ui;
}
