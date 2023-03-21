#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QDir>
#include <QFileSystemModel>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFileSystemModel *fileSystemModel = new QFileSystemModel;
    QModelIndex rootPathIndex = fileSystemModel->setRootPath("D:/resource/project/ubtEduTools/deviceManager/manager/bin/win32/debug/shared/");
    ui->listView->setModel(fileSystemModel);
    ui->listView->setRootIndex(rootPathIndex);
}

MainWindow::~MainWindow()
{
    delete ui;
}


