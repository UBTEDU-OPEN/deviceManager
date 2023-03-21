#include "aiboxResetConfirmProcedure.h"

#include "commondialog.h"
#include "warningwidget.h"
#include "saveUserDataWidget.h"

AiboxResetConfirmProcedure::AiboxResetConfirmProcedure(QObject *parent)
    : ConfirmProcedure(parent)
{}

void AiboxResetConfirmProcedure::startProcedure()
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
    warningwidget *warning = new warningwidget(QString::fromLocal8Bit("为保证设备安全恢复出厂必须连接电源，恢复过程中请不要关机和断开电源。"),dialog);
    dialog->setDisplayWidget(warning);
    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted) {
        if(accepted) {
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, dynamic_cast<QWidget*>(parent()));
            SaveUserDataWidget *saveUserDataWidget = new SaveUserDataWidget(QString::fromLocal8Bit("恢复后设备将会失去之前保留在设备上的数据，如果需要保留数据，请把数据备份到userdata目录下，并勾选保留userdata选项。"), true, dialog);
            connect(saveUserDataWidget, &SaveUserDataWidget::sigAccepted, this, &AiboxResetConfirmProcedure::confirmComplete);
            dialog->setDisplayWidget(saveUserDataWidget);
            connect(saveUserDataWidget, &SaveUserDataWidget::sigAccepted, dialog, &CommonDialog::close);
            dialog->setFixedSize(580,230);
            dialog->show();
        } else {
            emit confirmComplete(false, 0);
        }
    });
    dialog->show();
}
