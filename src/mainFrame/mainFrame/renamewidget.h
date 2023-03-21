#ifndef RENAMEWIDGET_H
#define RENAMEWIDGET_H

#include <QWidget>

namespace Ui {
class renamewidget;
}

class renamewidget : public QWidget
{
    Q_OBJECT

public:
    explicit renamewidget(QWidget *parent = nullptr);
    ~renamewidget();

    void onRename();

private:
    Ui::renamewidget *ui;

signals:
    void sigRename(QString name);
};

#endif // RENAMEWIDGET_H
