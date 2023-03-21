#ifndef TABCONTAINER_H
#define TABCONTAINER_H

#include <QWidget>
#include <QPushButton>
#include <QVector>

#include "tabwidget.h"

namespace Ui {
class TabContainer;
}

class TabContainer : public QWidget
{
    Q_OBJECT

public:
    explicit TabContainer(QWidget *parent = nullptr);
    ~TabContainer();

    void hideNavigationBtn();
    void showNavigationBtn();
    void hideAddBtn();
    void showAddBtn();
    void addTab(QString tabName);
    void resetTabIndex();
    void renameTab(int index, QString tabName);

signals:
    void tabClicked(int);
    void tabClose(int);
    void addClicked();
    void tabRenameRequest(int);

protected:
    void paintEvent(QPaintEvent *e) override;

private slots:
    void onPreClicked();
    void onNextClicked();
    void onTabClicked(int);
    void onTabClose(int);

private:
    Ui::TabContainer *ui;
    int currentIndex_;
    int visibleMin_;
    int visibleMax_;
    QVector<TabWidget*> tabWidgets_;
};

#endif // TABCONTAINER_H
