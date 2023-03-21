#include "fileCopyProgressDlg.h"
#include "ui_fileCopyProgressDlg.h"

#include "fileCopyThread.h"
#include "logHelper.h"

#include <QFile>

const QString FileCopyProgressDlg::QSS_PATH(":/res/qss/fileCopyProgressDlg.qss");

FileCopyProgressDlg::FileCopyProgressDlg(const QStringList &srcFiles, const QString &dstDir, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FileCopyProgressDlg)
{
    ui->setupUi(this);

    setModal(true);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setDefaultStyle();

    connect(ui->CancelBtn, &QPushButton::clicked, this, &FileCopyProgressDlg::onCancel);

    fileCopyThread_ = new FileCopyThread(this);
    fileCopyThread_->setSrcFiles(srcFiles);
    fileCopyThread_->setDstPath(dstDir);
    connect(fileCopyThread_, &FileCopyThread::copyStart, this, &FileCopyProgressDlg::onStart);
    connect(fileCopyThread_, &FileCopyThread::copyFinished, this, &FileCopyProgressDlg::onFinish);
    connect(fileCopyThread_, &FileCopyThread::copyProgressing, this, &FileCopyProgressDlg::onCpyProgressing);

    onStart();
    fileCopyThread_->start();
}

FileCopyProgressDlg::~FileCopyProgressDlg()
{
    delete ui;
}

void FileCopyProgressDlg::onCancel()
{
    if (fileCopyThread_) {
        fileCopyThread_->onCancelRequest();
        fileCopyThread_->wait();
    }
    done(QDialog::Rejected);
}

void FileCopyProgressDlg::onStart()
{
    ui->UploadingProgressBar->setValue(0);
    ui->UploadingProgressValue->setText(QString("uploading 0%"));
}

void FileCopyProgressDlg::onCpyProgressing(qint64 value)
{
    if (value < 0) {
        value = 0;
    } else if (value > 100) {
        value = 100;
    }
    ui->UploadingProgressBar->setValue(value);
    ui->UploadingProgressValue->setText(QString("uploading %1%").arg(value));
}

void FileCopyProgressDlg::onFinish(QStringList failedFiles)
{
    Q_UNUSED(failedFiles)
    done(QDialog::Accepted);
}

void FileCopyProgressDlg::setDefaultStyle()
{
    QFile styleSheet(QSS_PATH);
    if(styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    else {
        LOG(ERROR) << "FileCopyProgressDlg::setDefaultStyle open qss failed";
    }
}
