#include "cancelUpgradeConfirmDlg.h"
#include "ui_cancelUpgradeConfirmDlg.h"

#include "logHelper.h"

#include <QFile>

CancelUpgradeConfirmDlg::CancelUpgradeConfirmDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CancelUpgradeConfirmDlg)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    loadStyleSheet();
    setAttribute(Qt::WA_DeleteOnClose);

//    connect(ui->okBtn, &QPushButton::clicked, this, [this]() {
//        emit cancelUpgrade();
//        close();
//    });
//    connect(ui->cancelBtn, &QPushButton::clicked, this, [this]() {
//        close();
//    });
//    connect(ui->closeBtn, &QPushButton::clicked, this, [this]() {
//        close();
//    });
}

CancelUpgradeConfirmDlg::~CancelUpgradeConfirmDlg()
{
    delete ui;
}

void CancelUpgradeConfirmDlg::loadStyleSheet()
{
    QFile styleSheetFile(":/res/qss/cancelUpgradeConfirmDlg.qss");
    if (styleSheetFile.open(QFile::ReadOnly)) {
        auto styleSheetStr = styleSheetFile.readAll();
        setStyleSheet(styleSheetStr);
        styleSheetFile.close();
    } else {
        LOG(ERROR) << "open :/res/qss/cancelUpgradeConfirmDlg.qss failed";
    }
}
