#ifndef DEVICELISTVIEW_H
#define DEVICELISTVIEW_H

#include <QListView>
#include <QModelIndex>

class DeviceListView : public QListView
{
    Q_OBJECT
public:
    explicit DeviceListView(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

signals:
    void failureIconClicked(int);

private:
    bool pressOnFailureIcon(QPoint pos);
    QSize failureIconSize_;
};

#endif // DEVICELISTVIEW_H
