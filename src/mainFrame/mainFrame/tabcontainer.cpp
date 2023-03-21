#include "tabcontainer.h"
#include "ui_tabcontainer.h"

#include <QFile>
#include <QStyleOption>
#include <QStyle>
#include <QPainter>

const int kInvalidIndex = -1;

TabContainer::TabContainer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabContainer),
    currentIndex_(kInvalidIndex),
    visibleMin_(kInvalidIndex),
    visibleMax_(kInvalidIndex)
{
    ui->setupUi(this);

    QFile styleSheet(":/res/qss/tabContainer.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }

    hideNavigationBtn();
    hideAddBtn();
    connect(ui->addBtn,&QPushButton::clicked,this,&TabContainer::addClicked);
    connect(ui->preBtn,&QPushButton::clicked,this,&TabContainer::onPreClicked);
    connect(ui->nextBtn,&QPushButton::clicked,this,&TabContainer::onNextClicked);
}

TabContainer::~TabContainer()
{
    delete ui;
}

void TabContainer::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}

void TabContainer::hideNavigationBtn()
{
    ui->preBtn->setEnabled(false);
    ui->preBtn->setVisible(false);
    ui->nextBtn->setEnabled(false);
    ui->nextBtn->setVisible(false);
}

void TabContainer::showNavigationBtn()
{
    ui->preBtn->setEnabled(true);
    ui->preBtn->setVisible(true);
    ui->nextBtn->setEnabled(true);
    ui->nextBtn->setVisible(true);
}

void TabContainer::hideAddBtn()
{
    ui->addBtn->setEnabled(false);
    ui->addBtn->setVisible(false);
}

void TabContainer::showAddBtn()
{
    ui->addBtn->setEnabled(true);
    ui->addBtn->setVisible(true);
}

void TabContainer::onPreClicked()
{
    if(tabWidgets_.empty()) {
        return;
    }
    if(currentIndex_ > visibleMin_) {
        tabWidgets_[currentIndex_]->setTabSelected(false);
        --currentIndex_;
        tabWidgets_[currentIndex_]->setTabSelected(true);
        emit tabClicked(currentIndex_);
    } else if(currentIndex_ == visibleMin_ && currentIndex_ > 0) {
        tabWidgets_[visibleMin_ + 6]->setTabVisible(false);
        tabWidgets_[currentIndex_]->setTabSelected(false);
        --visibleMax_;
        --visibleMin_;
        currentIndex_ = visibleMin_;
        tabWidgets_[currentIndex_]->setTabSelected(true);
        tabWidgets_[currentIndex_]->setTabVisible(true);
        emit tabClicked(currentIndex_);
    } else if(currentIndex_ == 0) {
        return;
    }
}
void TabContainer::onNextClicked()
{
    if(tabWidgets_.empty()) {
        return;
    }
    int indexMax = tabWidgets_.size() - 1;
    if(currentIndex_ < visibleMax_) {
        tabWidgets_[currentIndex_]->setTabSelected(false);
        ++currentIndex_;
        tabWidgets_[currentIndex_]->setTabSelected(true);
        emit tabClicked(currentIndex_);
    } else if(currentIndex_ == visibleMax_ && currentIndex_ < indexMax) {
        tabWidgets_[visibleMax_-6]->setTabVisible(false);
        tabWidgets_[currentIndex_]->setTabSelected(false);
        ++visibleMax_;
        ++visibleMin_;
        currentIndex_ = visibleMax_;
        tabWidgets_[currentIndex_]->setTabSelected(true);
        tabWidgets_[currentIndex_]->setTabVisible(true);
        emit tabClicked(currentIndex_);
    } else if(currentIndex_ == indexMax) {
        return;
    }
}

void TabContainer::addTab(QString tabName)
{
    int tabCount = tabWidgets_.size();
    if(tabCount == 0) {
        showAddBtn();
    } else if(tabCount == 7) {
        showNavigationBtn();
    }

    if(tabCount > 0) {
        tabWidgets_[currentIndex_]->setTabSelected(false);
    }

    auto tabWidget = new TabWidget(tabName,tabCount,this);
    connect(tabWidget,&TabWidget::tabClicked,this,&TabContainer::onTabClicked);
    connect(tabWidget,&TabWidget::tabClosed,this,&TabContainer::onTabClose);
    connect(tabWidget,&TabWidget::tabDblClicked,this,&TabContainer::tabRenameRequest);
    tabWidgets_.push_back(tabWidget);
    tabCount = tabWidgets_.size();
    tabWidget->setTabSelected(true);
    auto hLayout = dynamic_cast<QHBoxLayout*>(layout());
    auto addIndex = hLayout->indexOf(ui->addBtn);
    hLayout->insertWidget(addIndex,tabWidget);

    if(tabCount > 7) {
        visibleMax_ = tabCount - 1;
        visibleMin_ = visibleMax_ - 6;
        currentIndex_ = visibleMax_;

        for(int i = 0; i < tabCount; ++i) {
            if(i >= visibleMin_ && i <= visibleMax_) {
                tabWidgets_[i]->setTabVisible(true);
            } else {
                tabWidgets_[i]->setTabVisible(false);
            }
        }
    } else {
        visibleMax_ = tabCount - 1;
        visibleMin_ = 0;
        currentIndex_ = visibleMax_;
    }
}

void TabContainer::resetTabIndex()
{
    if(tabWidgets_.empty()) {
        return;
    }

    if(currentIndex_ != kInvalidIndex) {
        tabWidgets_[currentIndex_]->setTabSelected(false);
    }

    visibleMin_ = 0;
    currentIndex_ = visibleMin_;
    tabWidgets_[currentIndex_]->setTabSelected(true);
    int tabCount = tabWidgets_.size();

    if(tabCount >= 7) {
        visibleMax_ = 6;
    } else {
        visibleMax_ = tabCount - 1;
    }

    for(int i = 0; i < tabCount; ++i) {
        if(i >= visibleMin_ && i <= visibleMax_) {
            tabWidgets_[i]->setTabVisible(true);
        } else {
            tabWidgets_[i]->setTabVisible(false);
        }
    }
}

void TabContainer::onTabClicked(int index)
{
    if(currentIndex_ != kInvalidIndex) {
        tabWidgets_[currentIndex_]->setTabSelected(false);
    }
    tabWidgets_[index]->setTabSelected(true);
    currentIndex_ = index;
    emit tabClicked(index);
}

void TabContainer::onTabClose(int index)
{
    int tabCount = tabWidgets_.size();
    if(currentIndex_ != index) {
        tabWidgets_[currentIndex_]->setTabSelected(false);
    }
    for(int i = index + 1; i < tabCount; ++i) {
        tabWidgets_[i]->setTabIndex(i-1);
    }

    auto tabWidget = tabWidgets_[index];
    tabWidget->deleteLater();
    tabWidgets_.remove(index);
    emit tabClose(index);
    tabCount = tabWidgets_.size();

    if(visibleMax_ < tabCount) { //如果后面还有元素，从后面显示一个
        tabWidgets_[visibleMax_]->setTabVisible(true);
    } else if(visibleMin_ > 0) { //后面没有了，从前面显示一个
        --visibleMin_;
        tabWidgets_[visibleMin_]->setTabVisible(true);
    } else if(visibleMin_ == 0) { //已经没有隐藏的了
        --visibleMax_; //visibleMax_要开始缩减
    }
    currentIndex_ = index - 1;
    if(currentIndex_ < 0) {
        currentIndex_ = 0;
    }
    if(tabCount != 0) {
        tabWidgets_[currentIndex_]->setTabSelected(true);
    }

    if(tabCount == 7) {
        hideNavigationBtn();
    } /*else if(tabCount == 0) {
        hideAddBtn();
    }*/
}

void TabContainer::renameTab(int index, QString tabName)
{
    tabWidgets_[index]->setTabName(tabName);
}
