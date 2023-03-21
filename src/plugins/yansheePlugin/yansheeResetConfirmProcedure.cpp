#include "yansheeResetConfirmProcedure.h"

#include "resetModeSelectionWidget.h"

#include "commondialog.h"
#include "warningwidget.h"
#include "saveUserDataWidget.h"

YansheeResetConfirmProcedure::YansheeResetConfirmProcedure(QObject *parent)
    : ConfirmProcedure(parent)
{}

void YansheeResetConfirmProcedure::startProcedure()
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton, dynamic_cast<QWidget*>(parent()));
    ResetModeSelectionWidget *resetModeSelectionWidget = new ResetModeSelectionWidget(dialog);
    connect(resetModeSelectionWidget, &ResetModeSelectionWidget::confirmed, this, [this](bool accepted, int mode){
        if (accepted) {
            if (mode == 0) {
                CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
                warningwidget *warning = new warningwidget(QString::fromLocal8Bit("为保证设备安全恢复出厂必须连接电源，恢复过程中请不要关机和断开电源。"),dialog);
                dialog->setDisplayWidget(warning);
                connect(dialog, &CommonDialog::sigAccepted, [this, mode](bool accepted) {
                    emit confirmComplete(accepted, mode);
                });
                dialog->show();
            } else {
                emit confirmComplete(accepted, mode);
            }
        } else {
            emit confirmComplete(accepted, 0);
        }
    });
    dialog->setDisplayWidget(resetModeSelectionWidget);
    connect(resetModeSelectionWidget, &ResetModeSelectionWidget::confirmed, dialog, &CommonDialog::close);
    dialog->setFixedSize(580, 230);
    dialog->show();
}
