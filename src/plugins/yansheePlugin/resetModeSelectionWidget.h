#ifndef RESETMODESELECTIONWIDGET_H
#define RESETMODESELECTIONWIDGET_H

#include <QWidget>

namespace Ui {
class ResetModeSelectionWidget;
}

class ResetModeSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResetModeSelectionWidget(QWidget *parent = nullptr);
    ~ResetModeSelectionWidget();

signals:
    void confirmed(bool, int);

private:
    Ui::ResetModeSelectionWidget *ui;
};

#endif // RESETMODESELECTIONWIDGET_H
