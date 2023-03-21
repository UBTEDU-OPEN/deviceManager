#ifndef COMMONDIALOG_H
#define COMMONDIALOG_H

#include "utilsGlobal.h"

#include <QDialog>

namespace Ui {
class CommonDialog;
}

class UTILS_EXPORT CommonDialog : public QDialog
{
    Q_OBJECT

public:
    enum ButtonType {
        NoButton = 0x1,
        OnlyOkButton = 0x2,
        OkCancelButton = 0x3
    };
    explicit CommonDialog(const QString& title,
                          ButtonType type = OkCancelButton,
                          QWidget *parent = nullptr);
    ~CommonDialog();

    void setDisplayWidget(QWidget* widget); //注意，widget和底下的text是互斥的，只能调用其中一个
    void setDisplayText(const QString& text);
    void setOkBtnText(const QString& text);
    void setCancelBtnText(const QString& text);
    void setOk2BtnText(const QString& text);

    void setCloseOnOk(bool closeOnOk);
    void setCloseOnClose(bool closeOnClose);

public slots:
    void onOkClicked();
    void onCancelClicked();
    void onOk2Clicked();
    void onCloseClicked();
    void onSetOkBtnEnabled(bool);
    void onSetFocusToOkBtn();

signals:
    void sigAccepted(bool);
    void sigClosed();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void hideBtn(QPushButton* btn);

private:
    Ui::CommonDialog *ui;
    bool leftButtonPressed_;
    QPoint lastPos_;
    bool closeOnOk_;
    bool closeOnClose_;
};

#endif // COMMONDIALOG_H
