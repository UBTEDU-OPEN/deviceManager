#ifndef EXPORTWIDGET_H
#define EXPORTWIDGET_H

#include <QWidget>

namespace Ui {
class exportwidget;
}

class exportwidget : public QWidget
{
    Q_OBJECT

public:
    explicit exportwidget(QString classroomfilepath, QWidget *parent = nullptr);
    ~exportwidget();

private:
    Ui::exportwidget *ui;
    QString m_filepath;

signals:
    void sigExport(QString path, QWidget *widget);

public:
    void selectFilePath();
    void onExport();
    QString getPath();
};

#endif // EXPORTWIDGET_H
