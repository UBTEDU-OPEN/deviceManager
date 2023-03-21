#include "aiboxUpgradeConfirmProcedure.h"

#include "commondialog.h"
#include "warningwidget.h"
#include "saveUserDataWidget.h"

AiboxUpgradeConfirmProcedure::AiboxUpgradeConfirmProcedure(QObject *parent)
    : ConfirmProcedure(parent)
{
}

void AiboxUpgradeConfirmProcedure::startProcedure()
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
    QString upgradeWarningMsg = tr("In order to ensure the safe upgrade of the device, it is necessary to confirm that the device is connected to the power supply.\
                                   And disconnect the power supply. After the upgrade, the device will lose the data previously kept on the device, please back up in advance.\
                                   \nDetermine to perform the upgrade to version ")/*.arg(localPackageVersion_)*/;
    warningwidget *warning = new warningwidget(upgradeWarningMsg, dialog);
    dialog->setDisplayWidget(warning);
    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted) {
        if (accepted) {
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, dynamic_cast<QWidget*>(parent()));
            SaveUserDataWidget *saveUserDataConfirmDlg = new SaveUserDataWidget(tr("After the upgrade, the device will lose the data previously retained on the device. If you need to retain the data, please back up the data to the userdata directory and check the retain userdata option."), true, dialog);
            connect(saveUserDataConfirmDlg, &SaveUserDataWidget::sigAccepted, this, &AiboxUpgradeConfirmProcedure::confirmComplete);
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
