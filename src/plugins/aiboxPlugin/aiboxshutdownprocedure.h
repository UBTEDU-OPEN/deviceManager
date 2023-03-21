#ifndef AIBOXSHUTDOWNPROCEDURE_H
#define AIBOXSHUTDOWNPROCEDURE_H

#include "devicePluginInterface.h"

class AiboxShutDownProcedure : public ConfirmProcedure
{
    Q_OBJECT
public:
    explicit AiboxShutDownProcedure(QObject *parent = nullptr);

    void startProcedure() override;
};

#endif // AIBOXSHUTDOWNPROCEDURE_H
