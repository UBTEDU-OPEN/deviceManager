#include "tabwidget.h"
#include "ui_tabwidget.h"

#include <QStyleOption>
#include <QStyle>
#include <QPainter>
#include <QDebug>

TabWidget::TabWidget(QString tabName, int tabIndex, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabWidget),
    tabIndex_(tabIndex)
{
    ui->setupUi(this);
    setTabName(tabName);
    setProperty("selected",true);
    connect(ui->close,&QPushButton::clicked,this,&TabWidget::onCloseClicked);
}

TabWidget::~TabWidget()
{
    delete ui;
}

void TabWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}

void TabWidget::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    qDebug() << "mousePressEvent";
    emit tabClicked(tabIndex_);
}

void TabWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    qDebug() << "mouseDoubleClickEvent";
    emit tabDblClicked(tabIndex_);
}

void TabWidget::onCloseClicked()
{
    emit tabClosed(tabIndex_);
}

void TabWidget::setTabIndex(int index)
{
    tabIndex_ = index;
}

void TabWidget::setTabName(const QString& name)
{
    ui->label->setText(name);
}

void TabWidget::setTabVisible(bool visible)
{
    ui->close->setEnabled(visible);
    setVisible(visible);
}

void TabWidget::setTabSelected(bool selected)
{
    setProperty("selected",selected);
    style()->unpolish(this);
    style()->polish(this);

}
