#include "exportwidget.h"
#include "ui_exportwidget.h"

#include <QFileDialog>

exportwidget::exportwidget(QString classroomfilepath, QWidget *parent) :
    QWidget(parent),
    m_filepath(classroomfilepath),
    ui(new Ui::exportwidget)
{
    ui->setupUi(this);

    setFixedSize(580,150);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    QFile styleSheet(":/res/qss/exportwidget.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }
    ui->lbpath->setText(m_filepath);


    connect(ui->pbclose, &QPushButton::clicked, this, &exportwidget::close);
    connect(ui->pbcancel, &QPushButton::clicked, this, &exportwidget::close);
    connect(ui->pbselectpath, &QPushButton::clicked, this, &exportwidget::selectFilePath);
    connect(ui->pbok, &QPushButton::clicked, this, &exportwidget::onExport);
}

exportwidget::~exportwidget()
{
    delete ui;
}

void exportwidget::selectFilePath()
{
    auto newPath = QFileDialog::getExistingDirectory(this, tr("save to"), m_filepath);
    if(!newPath.isEmpty()) {
        m_filepath = newPath;
        ui->lbpath->setText(m_filepath);
    }
}

void exportwidget::onExport()
{
    emit sigExport(m_filepath, this);
}

QString exportwidget::getPath()
{
    return m_filepath;
}
