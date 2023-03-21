#ifndef AIBOXUPGRADECONFIRMPROCEDURE_H
#define AIBOXUPGRADECONFIRMPROCEDURE_H

#include "devicePluginInterface.h"

class AiboxUpgradeConfirmProcedure : public ConfirmProcedure
{
    Q_OBJECT

public:
    explicit AiboxUpgradeConfirmProcedure(QObject *parent = nullptr);

    void startProcedure() override;

};

#endif // AIBOXUPGRADECONFIRMPROCEDURE_H
