#include "fileShareWidget.h"
#include "ui_fileShareWidget.h"

#include "settings.h"
#include "sharedFileListItemDelegate.h"
#include "logHelper.h"
#include "fileSelectionWidget.h"
#include "commondialog.h"

#include <QFileSystemModel>

const QString FileShareWidget::QSS_PATH(":/res/qss/fileShareWidget.qss");

FileShareWidget::FileShareWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileShareWidget)
{
    ui->setupUi(this);

    ui->SharedFilesListView->setSelectionMode(QAbstractItemView::NoSelection);
    ui->SharedFilesListView->setFocusPolicy(Qt::NoFocus);
    ui->SharedFilesListView->setAlternatingRowColors(true);

    setDefaultStyle();

    auto sharedFolderPath = Settings::sharedFolderAbsPath();
    QDir sharedFolder(sharedFolderPath);
    if (!sharedFolder.exists()) {
        sharedFolder.mkpath(".");
    }

    QFileSystemModel *model = new QFileSystemModel;
    model->setRootPath(sharedFolder.absolutePath());
    model->sort(3, Qt::DescendingOrder);
    ui->SharedFilesListView->setModel(model);
    sharedFileListItemDelegate_ = new SharedFileListItemDelegate(model);
    ui->SharedFilesListView->setItemDelegate(sharedFileListItemDelegate_);
    ui->SharedFilesListView->setRootIndex(model->index(sharedFolder.absolutePath()));
    ui->SharedFilesListView->setMouseTracking(true);

    connect(sharedFileListItemDelegate_, &SharedFileListItemDelegate::deleteRequest, this, &FileShareWidget::onDeleteRequest);
    connect(ui->AddBtn, &QPushButton::clicked, this, &FileShareWidget::onASddShareFileClicked);
}

FileShareWidget::~FileShareWidget()
{
    delete ui;
}

void FileShareWidget::onASddShareFileClicked()
{
    CommonDialog *dialog = new CommonDialog(tr("add share file"), CommonDialog::OkCancelButton, this);
    FileSelectionWidget *fileSelectionWidget = new FileSelectionWidget;
//    connect(dialog ,&CommonDialog::sigAccepted, widget, &AddDevicesWidget::onAccepted);
    dialog->setDisplayWidget(fileSelectionWidget);
    dialog->setOkBtnText(tr("upload"));
    dialog->onSetOkBtnEnabled(false);
    dialog->setCloseOnOk(false);
    connect(fileSelectionWidget, &FileSelectionWidget::uploadFileSelecteChanged, dialog, [dialog](bool selected) {
        dialog->onSetOkBtnEnabled(selected);
    });
    connect(fileSelectionWidget, &FileSelectionWidget::closeRequest, dialog, &CommonDialog::close);
    connect(fileSelectionWidget, &FileSelectionWidget::fileUploaded, this, &FileShareWidget::shareListChanged);
    connect(dialog, &CommonDialog::sigAccepted, fileSelectionWidget, &FileSelectionWidget::onUploadClicked);
    dialog->exec();
}

void FileShareWidget::onDeleteRequest(const QModelIndex index)
{
    QFileSystemModel *fileSystemModel = dynamic_cast<QFileSystemModel*>(ui->SharedFilesListView->model());
    LOG(INFO) << "delete " << fileSystemModel->filePath(index).toStdString() << "requested";
    QFile file(fileSystemModel->filePath(index));
    file.remove();
    emit shareListChanged();
}

void FileShareWidget::setDefaultStyle()
{
    QFile styleSheet(QSS_PATH);
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    else {
        LOG(ERROR) << "FileShareWidget::setDefaultStyle open qss failed";
    }
}
