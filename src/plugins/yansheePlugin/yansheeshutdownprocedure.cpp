#include "yansheeshutdownprocedure.h"

#include "commondialog.h"
#include "warningwidget.h"

YansheeShutDownProcedure::YansheeShutDownProcedure(QObject *parent)
    : ConfirmProcedure(parent)
{

}

void YansheeShutDownProcedure::startProcedure()
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
    QString warningStr = tr("Are you sure you need to perform a batch shutdown operation?\n\n"
                            "Note: Please make sure that the robot has been placed in a safe location,"
                            "avoid damage to the robot from falling down after power off.");
    warningwidget *warning = new warningwidget(warningStr,dialog);
    dialog->setDisplayWidget(warning);
    dialog->setMinimumSize(580,130);
    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted){
        emit confirmComplete(accepted,0);
    });
    dialog->show();
}
