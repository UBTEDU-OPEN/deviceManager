#ifndef YANSHEESHUTDOWNPROCEDURE_H
#define YANSHEESHUTDOWNPROCEDURE_H

#include "devicePluginInterface.h"

class YansheeShutDownProcedure : public ConfirmProcedure
{
    Q_OBJECT
public:
    explicit YansheeShutDownProcedure(QObject *parent = nullptr);

    void startProcedure() override;
};

#endif // YANSHEESHUTDOWNPROCEDURE_H
