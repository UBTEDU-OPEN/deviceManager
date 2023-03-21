#ifndef YANSHEERESETCONFIRMPROCEDURE_H
#define YANSHEERESETCONFIRMPROCEDURE_H

#include "devicePluginInterface.h"

class YansheeResetConfirmProcedure : public ConfirmProcedure
{
    Q_OBJECT

public:
    explicit YansheeResetConfirmProcedure(QObject *parent = nullptr);

    void startProcedure() override;
};

#endif // YANSHEERESETCONFIRMPROCEDURE_H
