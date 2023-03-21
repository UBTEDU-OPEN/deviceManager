#ifndef RESTARTTOUPGRADEDLG_H
#define RESTARTTOUPGRADEDLG_H

#include <QDialog>

namespace Ui {
class RestartToUpgradeDlg;
}

class RestartToUpgradeDlg : public QDialog
{
    Q_OBJECT

public:
    explicit RestartToUpgradeDlg(QWidget *parent = nullptr);
    ~RestartToUpgradeDlg();

signals:
    void restartToUpgrade();

private:
    Ui::RestartToUpgradeDlg *ui;
};

#endif // RESTARTTOUPGRADEDLG_H
