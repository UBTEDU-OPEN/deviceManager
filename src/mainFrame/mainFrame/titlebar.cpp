#include "titlebar.h"
#include "ui_titlebar.h"

#include <QPushButton>
#include <QAction>
#include <QMouseEvent>
#include <QDebug>

TitleBar::TitleBar(QWidget *parent)
	: QWidget(parent)
    , ui(new Ui::titlebar)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_StyledBackground);
    QFile styleSheet(":/res/qss/titleBar.qss");
    if(styleSheet.open(QIODevice::ReadOnly))
    {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
        
    connect(ui->file, &QPushButton::clicked, this, &TitleBar::sigFileTrigger);
    connect(ui->help, &QPushButton::clicked, this, &TitleBar::sigHelpTrigger);
	connect(ui->min, &QPushButton::clicked, this, &TitleBar::sigMin);
	connect(ui->max, &QPushButton::clicked, this, &TitleBar::sigMax);
	connect(ui->close, &QPushButton::clicked, this, &TitleBar::sigClose);
}

TitleBar::~TitleBar()
{
	delete ui;
}

QPoint TitleBar::getFileBtnBottomLeft()
{
    QPoint bottomLeft = ui->file->geometry().bottomLeft();
    return mapToParent(bottomLeft);
}

QPoint TitleBar::getHelpBtnBottomLeft()
{
    QPoint bottomLeft = ui->help->geometry().bottomLeft();
    return mapToParent(bottomLeft);
}
