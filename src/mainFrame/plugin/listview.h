#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QWidget>
#include <QListView>

class ListView : public QListView
{
    Q_OBJECT
public:
    explicit ListView(QWidget *parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    int lastRow_;
    QModelIndex lastIndex_;
};

#endif // LISTVIEW_H
