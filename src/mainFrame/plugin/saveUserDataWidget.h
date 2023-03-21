#ifndef SAVEUSERDATACONFIRMDLG_H
#define SAVEUSERDATACONFIRMDLG_H

#include "pluginGlobal.h"

#include <QWidget>

namespace Ui {
class SaveUserDataWidget;
}

class PLUGIN_EXPORT SaveUserDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SaveUserDataWidget(const QString &warning, bool saveUserData, QWidget *parent = nullptr);
    ~SaveUserDataWidget();
    void hideSaveUserData();

signals:
    void sigAccepted(bool, int);

private:
    Ui::SaveUserDataWidget *ui;
};

#endif // SAVEUSERDATACONFIRMDLG_H
