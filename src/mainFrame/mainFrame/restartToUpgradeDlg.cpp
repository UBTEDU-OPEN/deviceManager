#include "restartToUpgradeDlg.h"
#include "ui_restartToUpgradeDlg.h"

RestartToUpgradeDlg::RestartToUpgradeDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RestartToUpgradeDlg)
{
    ui->setupUi(this);

    connect(ui->cancelBtn, &QPushButton::clicked, this, [this]() {
        close();
    });
    connect(ui->restartBtn, &QPushButton::clicked, this, [this]() {
        emit restartToUpgrade();
        close();
    });
}

RestartToUpgradeDlg::~RestartToUpgradeDlg()
{
    delete ui;
}
