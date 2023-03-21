#ifndef WARNINGWIDGET_H
#define WARNINGWIDGET_H

#include "utilsGlobal.h"

#include <QWidget>

namespace Ui {
class warningwidget;
}

class UTILS_EXPORT warningwidget : public QWidget
{
    Q_OBJECT

public:
    explicit warningwidget(QString text, QWidget *parent = nullptr);
    ~warningwidget();

private:
    Ui::warningwidget *ui;
};

#endif // WARNINGWIDGET_H
