#ifndef ADDDEVICESDIALOG_H
#define ADDDEVICESDIALOG_H

#include <QWidget>
#include <QList>
#include <QPair>

namespace Ui {
class AddDevicesWidget;
}

class AddDevicesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddDevicesWidget(int maxNum, QWidget *parent = nullptr);
    ~AddDevicesWidget();

public slots:
    void onRadioBtnClicked(int);
    void onChoose();
    void previewSn2Id();
    void onAccepted(bool);
    void onTextChanged();
    void onEditingFinished();

signals:
    void addDevices(QList<QPair<QString, QString>>);
    void setOkBtnEnabled(bool);
    void setFocusToOkBtn();

private:
    Ui::AddDevicesWidget *ui;
    int selectedId_;
    QList<QPair<QString, QString>> sns2ids_;
    int maxNum_; //还能导入的设备最大数量
};

#endif // ADDDEVICESDIALOG_H
