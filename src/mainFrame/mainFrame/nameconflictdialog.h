#ifndef NAMECONFLICTDIALOG_H
#define NAMECONFLICTDIALOG_H

#include <QDialog>

namespace Ui {
class NameConflictDialog;
}

class NameConflictDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NameConflictDialog(QWidget *parent = nullptr);
    ~NameConflictDialog();

    void setWarningMsg(const QString& msg);

private:
    Ui::NameConflictDialog *ui;
};

#endif // NAMECONFLICTDIALOG_H
