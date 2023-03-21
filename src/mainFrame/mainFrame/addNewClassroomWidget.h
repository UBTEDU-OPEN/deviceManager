#ifndef ADDNEWCLASSROOMWIDGET_H
#define ADDNEWCLASSROOMWIDGET_H

#include <QWidget>

namespace Ui {
class AddNewClassroomWidget;
}

class AddNewClassroomWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddNewClassroomWidget(QWidget *parent = nullptr);
    ~AddNewClassroomWidget();

signals:
    void addNewClassroomRequest();

private:
    Ui::AddNewClassroomWidget *ui;
};

#endif // ADDNEWCLASSROOMWIDGET_H
