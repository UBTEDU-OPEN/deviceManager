#ifndef BACKGROUNDWIGET_H
#define BACKGROUNDWIGET_H

#include <QWidget>

namespace Ui {
class BackgroundWiget;
}

class BackgroundWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BackgroundWidget(QWidget *parent = nullptr);
    ~BackgroundWidget();

signals:
    void addBtnClicked();

private:
    Ui::BackgroundWiget *ui;
};

#endif // BACKGROUNDWIGET_H
