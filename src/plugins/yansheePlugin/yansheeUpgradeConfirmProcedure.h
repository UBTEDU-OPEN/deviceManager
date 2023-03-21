#ifndef YANSHEEUPGRADECONFIRMPROCEDURE_H
#define YANSHEEUPGRADECONFIRMPROCEDURE_H

#include "devicePluginInterface.h"

class YansheeUpgradeConfirmProcedure : public ConfirmProcedure
{
    Q_OBJECT

public:
    explicit YansheeUpgradeConfirmProcedure(QObject *parent = nullptr);

    void startProcedure() override;
    void hideSaveUserData() override { displaySaveUserData_ = false; }

private:
    bool displaySaveUserData_;

};

#endif // YANSHEEUPGRADECONFIRMPROCEDURE_H
