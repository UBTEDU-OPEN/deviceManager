#ifndef AIBOXRESETCONFIRMPROCEDURE_H
#define AIBOXRESETCONFIRMPROCEDURE_H

#include "devicePluginInterface.h"

class AiboxResetConfirmProcedure : public ConfirmProcedure
{
    Q_OBJECT

public:
    explicit AiboxResetConfirmProcedure(QObject *parent = nullptr);

    void startProcedure() override;
};

#endif // AIBOXRESETCONFIRMPROCEDURE_H
