#include "classroomContainerTabBar.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QToolButton>
#include <QPushButton>

#include <QDebug>

ClassroomContainerTabBar::ClassroomContainerTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setMouseTracking(true);
//    setTabsClosable(true);
    setDrawBase(false);

//    setUsesScrollButtons(false);
//    lBtn_ = new QPushButton(this);
//    lBtn_->setFixedSize(20, 20);
//    lBtn_->move(20,0);
//    lBtn_->setAttribute(Qt::WA_AlwaysStackOnTop);

//    rBtn_ = new QPushButton(this);
//    rBtn_->setFixedSize(20, 20);
//    rBtn_->move(600,0);
//    rBtn_->setAttribute(Qt::WA_AlwaysStackOnTop);

    connect(this, &ClassroomContainerTabBar::tabBarDoubleClicked, this, &ClassroomContainerTabBar::renameRequest);
}

void ClassroomContainerTabBar::mousePressEvent(QMouseEvent *me)
{
    if (tabRect(count() - 1).contains(me->pos())) {
        me->accept();
        emit newTabRequest();
    } else {
        QTabBar::mousePressEvent(me);
    }
    int index = tabAt(me->pos());
    if (index != (count() -1) && tabBtnRect(index).contains(me->pos())) {
        emit buttonClicked(index);
    }
}

void ClassroomContainerTabBar::paintEvent(QPaintEvent* event)
{
    QTabBar::paintEvent(event);
    QPainter painter(this);
    painter.save();

    for (int i = 0; i < (count() -1); ++i) {
        if (tabBtnHover(i)) {
            painter.drawPixmap(tabBtnRect(i),QPixmap(":/res/images/ic_close_tab_p.svg"));
        } else {
           painter.drawPixmap(tabBtnRect(i),QPixmap(":/res/images/ic_close_tab.svg"));
        }
    }
    painter.restore();

}

void ClassroomContainerTabBar::mouseMoveEvent(QMouseEvent *event)
{
    QTabBar::mouseMoveEvent(event);
    pos_ = event->pos();
    int index = tabAt(pos_);
    if(tabBtnHover(index)) {
        if(index != hoverIndex_) {
            update();
        }
        hoverIndex_ = index;
    } else {
        if(hoverIndex_ != -1) {
            update();
        }
        hoverIndex_ = -1;
    }
}

void ClassroomContainerTabBar::resizeEvent(QResizeEvent *re)
{
    QTabBar::resizeEvent(re);
}

QRect ClassroomContainerTabBar::tabBtnRect(int index)
{
    QRect rect = tabRect(index);
    rect.setX(rect.x() + rect.width() - 30 - 9); //9是margin
    rect.setY(rect.y() + 9 + 9); //(36-18)/2+margin
    rect.setWidth(18);
    rect.setHeight(18);
    return rect;
}

bool ClassroomContainerTabBar::tabBtnHover(int index)
{
    if(tabBtnRect(index).contains(pos_)) {
        return true;
    }
    return false;
}

