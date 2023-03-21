#include "yansheeUpgradeConfirmProcedure.h"

#include "commondialog.h"
#include "warningwidget.h"
#include "saveUserDataWidget.h"

YansheeUpgradeConfirmProcedure::YansheeUpgradeConfirmProcedure(QObject *parent)
    : ConfirmProcedure(parent)
    , displaySaveUserData_(true)
{
}

void YansheeUpgradeConfirmProcedure::startProcedure()
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
    QString upgradeWarningMsg;
    if(displaySaveUserData_) {//全量包需要选择勾选
        upgradeWarningMsg = tr("To ensure the safe upgrade of the device,"
                               " it must be connected to the power source."
                               " Please do not shut down or disconnect the "
                               "power source during the upgrade process. Note:"
                               " The processing time of using the complete upgrade"
                               " package is longer (more than 60 minutes/unit),"
                               " and it is recommended to use the differential package to upgrade.");
    } else {//差分包不需要
        upgradeWarningMsg = tr("In order to ensure the safe upgrade of the device,"
                               " it is necessary to ensure that the device and the power supply remain connected."
                               " Please do not shut down or disconnect the power supply during the process. "
                               "And after the upgrade, the device will lose the data previously kept on the device, "
                               "please back it up in advance.");
    }

    warningwidget *warning = new warningwidget(upgradeWarningMsg, dialog);
    dialog->setDisplayWidget(warning);
    dialog->setOkBtnText(tr("continue"));
    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted) {
        if (accepted) {
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, dynamic_cast<QWidget*>(parent()));
            QString msg;
            msg = tr("After the upgrade, the device will lose the data previously retained on the device. "
                     "If you need to retain the data, please back up the data to the userdata directory "
                     "and check the retain userdata option.");
            if(!displaySaveUserData_) {
                msg = tr("After the upgrade, the device will lose the data previously retained on the device. "
                         "If you need to retain the data, please back up the data to the userdata directory.");
            }
            SaveUserDataWidget *saveUserDataConfirmDlg = new SaveUserDataWidget(msg, true, dialog);
            if(!displaySaveUserData_) {
                saveUserDataConfirmDlg->hideSaveUserData();
            }
            connect(saveUserDataConfirmDlg, &SaveUserDataWidget::sigAccepted, this, &YansheeUpgradeConfirmProcedure::confirmComplete);
            dialog->setDisplayWidget(saveUserDataConfirmDlg);
            connect(saveUserDataConfirmDlg, &SaveUserDataWidget::sigAccepted, dialog, &CommonDialog::close);
            dialog->setFixedSize(580,230);
            dialog->show();
        } else {
            emit confirmComplete(false, 0);
        }
    });

    dialog->show();
}
