#include "fileSelectionWidget.h"
#include "ui_fileSelectionWidget.h"

#include "logHelper.h"
#include "settings.h"
#include "fileCopyProgressDlg.h"

#include <QFile>
#include <QFileDialog>
#include <QDebug>

const QString FileSelectionWidget::QSS_PATH(":/res/qss/fileSelectionWidget.qss");

FileSelectionWidget::FileSelectionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileSelectionWidget)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);

    setDefaultStyle();

    connect(ui->ShareFileSelectButton, &QPushButton::clicked, this, &FileSelectionWidget::onSelectClicked);
}

FileSelectionWidget::~FileSelectionWidget()
{
    delete ui;
}

void FileSelectionWidget::onSelectClicked()
{
    auto filesPathe = QFileDialog::getOpenFileName();
    if (filesPathe.isEmpty()) {
        ui->ShareFileNameLabel->setText(tr("select file to share"));
        ui->FileIconLabel->setPixmap(QPixmap(":/res/images/ic_folder.png"));
        ui->ShareFileSelectButton->setText(tr("select"));
        filePath_.clear();
        emit uploadFileSelecteChanged(false);
    } else {
        QFileInfo fileInfo(filesPathe);
        ui->ShareFileNameLabel->setText(fileInfo.fileName());
        ui->FileIconLabel->setPixmap(QPixmap(":/res/images/ic_file.svg"));
        ui->ShareFileSelectButton->setText(tr("reselect"));
        filePath_ = fileInfo.absoluteFilePath();
        emit uploadFileSelecteChanged(true);
    }
}

void FileSelectionWidget::onUploadClicked(bool accepted)
{
    if (accepted) {
        FileCopyProgressDlg *fileCopyProgressDlg = new FileCopyProgressDlg({ filePath_ }, Settings::sharedFolderAbsPath());
        if (QDialog::Accepted == fileCopyProgressDlg->exec()) {
            emit closeRequest();
            emit fileUploaded();
        }
    }
}

void FileSelectionWidget::setDefaultStyle()
{
    QFile styleSheet(QSS_PATH);
    if(styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    else {
        LOG(ERROR) << "FileSelectionWidget::setDefaultStyle open qss failed";
    }
}
