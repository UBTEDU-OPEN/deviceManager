#include "aiboxshutdownprocedure.h"

#include "commondialog.h"
#include "warningwidget.h"

AiboxShutDownProcedure::AiboxShutDownProcedure(QObject *parent)
    : ConfirmProcedure(parent)
{

}

void AiboxShutDownProcedure::startProcedure()
{
    CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OkCancelButton, dynamic_cast<QWidget*>(parent()));
    QString warningStr = tr("Are you sure you need to perform a batch shutdown operation?");
    warningwidget *warning = new warningwidget(warningStr,dialog);
    dialog->setDisplayWidget(warning);
    dialog->setMinimumSize(580,130);
    connect(dialog, &CommonDialog::sigAccepted, [this](bool accepted){
        emit confirmComplete(accepted,0);
    });
    dialog->show();
}
