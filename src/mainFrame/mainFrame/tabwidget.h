#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QWidget>

namespace Ui {
class TabWidget;
}

class TabWidget : public QWidget
{
    Q_OBJECT

public:
    TabWidget(QString tabName, int tabIndex, QWidget *parent = nullptr);
    ~TabWidget();
    void setTabIndex(int index);
    void setTabName(const QString& name);
    void setTabVisible(bool visible);
    void setTabSelected(bool selected);

signals:
    void tabClicked(int);
    void tabClosed(int);
    void tabDblClicked(int);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
    void onCloseClicked();

private:
    Ui::TabWidget *ui;
    int tabIndex_;
};

#endif // TABWIDGET_H
