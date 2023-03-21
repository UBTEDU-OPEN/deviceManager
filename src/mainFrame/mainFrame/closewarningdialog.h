#ifndef CLOSEWARNINGDIALOG_H
#define CLOSEWARNINGDIALOG_H

#include <QDialog>

namespace Ui {
class CloseWarningDialog;
}

class CloseWarningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloseWarningDialog(QWidget *parent = nullptr);
    ~CloseWarningDialog();

signals:
    void sigAccepted(bool);

private:
    Ui::CloseWarningDialog *ui;
};

#endif // CLOSEWARNINGDIALOG_H
