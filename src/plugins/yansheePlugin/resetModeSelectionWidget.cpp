#include "resetModeSelectionWidget.h"
#include "ui_resetModeSelectionWidget.h"

#include <QButtonGroup>

ResetModeSelectionWidget::ResetModeSelectionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ResetModeSelectionWidget)
{
    ui->setupUi(this);

    QButtonGroup *modeSelection = new QButtonGroup(this);
    modeSelection->addButton(ui->slightRecovery, 1);
    modeSelection->addButton(ui->completeRecovery, 0);

    connect(ui->okBtn, &QPushButton::clicked, [this, modeSelection] {
        emit confirmed(true, modeSelection->checkedId());
        close();
    });
    connect(ui->cancelBtn, &QPushButton::clicked, [this, modeSelection] {
        emit confirmed(false, modeSelection->checkedId());
        close();
    });

    ui->cancelBtn->setStyleSheet("background-color:#5B6479;border-radius:3px;color:#FFFFFF");
    ui->okBtn->setStyleSheet("background-color:#00A5FF;border-radius:3px;color:#FFFFFF");
    ui->slightRecovery->setStyleSheet("color:#FFFFFF");
    ui->completeRecovery->setStyleSheet("color:#FFFFFF");
}

ResetModeSelectionWidget::~ResetModeSelectionWidget()
{
    delete ui;
}
