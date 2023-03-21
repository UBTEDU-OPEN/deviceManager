#include "addNewClassroomWidget.h"
#include "ui_addNewClassroomWidget.h"

AddNewClassroomWidget::AddNewClassroomWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddNewClassroomWidget)
{
    ui->setupUi(this);

    connect(ui->addBtn, &QPushButton::clicked, this, &AddNewClassroomWidget::addNewClassroomRequest);
}

AddNewClassroomWidget::~AddNewClassroomWidget()
{
    delete ui;
}
