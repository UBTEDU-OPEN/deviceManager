#ifndef CANCELUPGRADECONFIRMDLG_H
#define CANCELUPGRADECONFIRMDLG_H

#include "utilsGlobal.h"

#include <QDialog>

namespace Ui {
class CancelUpgradeConfirmDlg;
}

class UTILS_EXPORT CancelUpgradeConfirmDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CancelUpgradeConfirmDlg(QWidget *parent = nullptr);
    ~CancelUpgradeConfirmDlg();

protected:
    void loadStyleSheet();

signals:
    void cancelUpgrade();

private:
    Ui::CancelUpgradeConfirmDlg *ui;
};

#endif // CANCELUPGRADECONFIRMDLG_H
