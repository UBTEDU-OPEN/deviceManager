#ifndef PLATFORMSELECTIONDIALOG_H
#define PLATFORMSELECTIONDIALOG_H

#include <QDialog>
#include <QPushButton>

class PluginManager;
class DevPluginInterface;

namespace Ui {
class PlatformSelectionDialog;
}

class PlatformSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlatformSelectionDialog(const PluginManager *pluginManager, QWidget *parent = nullptr);
    ~PlatformSelectionDialog();

public slots:
    void onSelection();
    void onCloseClicked();

protected:
//  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent *event);

private:
    void addDevice(const DevPluginInterface *devicePlugin);

signals:
    void selection(const QString& name);

private:
    const PluginManager *pluginManager_;

    Ui::PlatformSelectionDialog *ui;

    QPushButton *closeBtn_;
};

#endif // PLATFORMSELECTIONDIALOG_H
