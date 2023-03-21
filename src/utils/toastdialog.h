#ifndef TOASTDIALOG_H
#define TOASTDIALOG_H

#include "utilsGlobal.h"

#include <QDialog>

namespace Ui {
class ToastDialog;
}

class UTILS_EXPORT ToastDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToastDialog(QWidget *parent = nullptr);
    ~ToastDialog();
    void setDisplayText(const QString& text);

private:
    Ui::ToastDialog *ui;
};

#endif // TOASTDIALOG_H
