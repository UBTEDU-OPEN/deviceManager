#include "mainFrame.h"
#include "pluginManager.h"
#include "settings.h"
#include "config.h"
#include "logHelper.h"
#include "upgrade.h"
#include "platformselectiondialog.h"
#include "classroomContainer.h"
#include "fileDirHandler.h"
#include "titlebar.h"
#include "exportwidget.h"
#include "aboutwidget.h"
#include "renamewidget.h"
#include "closewarningdialog.h"
#include "nameconflictdialog.h"
#include "commondialog.h"
#include "warningwidget.h"
#include "commonMacro.h"
#include "aboutDlg.h"
#include "toastdialog.h"
#include "tabcontainer.h"

#include <QMenuBar>
#include <QAction>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QFileDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QCloseEvent>
#include <QString>
#include <QTabBar>
#include <QInputDialog>
#include <QFrame>
#include <QDir>
#include <QDesktopWidget>
#include <QApplication>
#include <QMessageBox>
#include <QTranslator>

#include <QDebug>

QMap<QString, int> MainFrame::classroomFileIndex;

QString MainFrameVersion { STR_MACRO(MAINFRAME_VERSION) };

MainFrame::MainFrame(QWidget *parent)
    : QMainWindow(parent)
    , pluginManager_(nullptr)
    , exportAction_(nullptr)
    , saveAction_(nullptr)
    , saveAllAction_(nullptr)
    , lan_(Language::Unset)
    , lBtnPressed_(false)
{
    LOG(WARNING) << "################### Main Frame Version:" << MainFrameVersion.toStdString();

    auto desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->screenGeometry();

    setWindowFlag(Qt::FramelessWindowHint);
    setFixedSize(screenRect.width() * 78 / 100, screenRect.height() * 71 / 100);
    setObjectName("mainFrame");

    QFile styleSheet(":/res/qss/main_frame.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
        styleSheet.close();
    }

    pluginManager_ = new PluginManager(this);
    setLanguage(Language::Cn);

    buildMenuBar();

    tabContainer_ = new TabContainer(this);
    connect(tabContainer_,&TabContainer::addClicked,this,&MainFrame::onNewTabRequest);
    connect(tabContainer_,&TabContainer::tabClicked,this,&MainFrame::onTabClicked);
    connect(tabContainer_,&TabContainer::tabClose,this,&MainFrame::onCloseRequest);
    connect(tabContainer_,&TabContainer::tabRenameRequest,this,&MainFrame::onRenameRequest);
    auto tabConainerLayout = new QHBoxLayout(tabContainer_);
    Q_UNUSED(tabConainerLayout);
    tabContainer_->setObjectName("tabContainer");
    tabWidgetContainer_ = new QWidget(this);
    tabWidgetContainer_->setObjectName("tabWidgetContainer");
    auto tabWidgetConainerLayout = new QHBoxLayout(tabWidgetContainer_);

    classroomContainer_ = new ClassroomContainer;
    classroomContainer_->tabBar()->hide();
    QFrame* frame = new QFrame(this);
    frame->setObjectName("classroomContainerFrame");
    auto frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(30,0,30,0);
    frameLayout->setSpacing(2);
    frameLayout->addWidget(tabContainer_);
    frameLayout->addWidget(tabWidgetContainer_);

    tabWidgetConainerLayout->addWidget(classroomContainer_);
    tabWidgetConainerLayout->setContentsMargins(0,0,0,0);

    QWidget* widget = new QWidget(this);
    QVBoxLayout* vLayout = new QVBoxLayout(widget);
    vLayout->setContentsMargins(0, 0, 0, 30);
    titleBar_ = new TitleBar(widget);
    connect(titleBar_,&TitleBar::sigClose,this,&MainFrame::onClose);
    connect(titleBar_,&TitleBar::sigMax,this,&MainFrame::onMax);
    connect(titleBar_,&TitleBar::sigMin,this,&MainFrame::onMin);
    connect(titleBar_,&TitleBar::sigFileTrigger,this,&MainFrame::onFileTrigger);
    connect(titleBar_,&TitleBar::sigHelpTrigger,this,&MainFrame::onHelpTrigger);
    vLayout->addWidget(titleBar_);
    vLayout->addWidget(frame);
    setCentralWidget(widget);

    connect(classroomContainer_, &ClassroomContainer::newTabRequest, this, &MainFrame::onNewTabRequest);
    connect(classroomContainer_, &ClassroomContainer::renameRequest, this, &MainFrame::onRenameRequest);
    connect(classroomContainer_, &ClassroomContainer::closeRequest, this, &MainFrame::onCloseRequest);
//    connect(classroomContainer_, &ClassroomContainer::currentChanged, this, &MainFrame::onCurrentClassroomChanged);
    connect(classroomContainer_, &ClassroomContainer::tabAdded, this, &MainFrame::onTabAdded);
    auto classrooms = Settings::openedClassroomsRecord();
    if (classrooms.empty()) {
        onNewTabRequest();
    } else {
        for (auto &classroom : classrooms) {
            QFileInfo classroomFileInfo(classroom);
            if (auto classroomWidget = pluginManager_->load(classroom)) {
                classroomContainer_->addClassroom(classroomWidget, classroomFileInfo.absoluteFilePath());
            }
        }

    }
    classroomContainer_->setCurrentIndex(0); //这里的0不能修改，底下的container的reset只是设置到0,即和底下配套使用，否则tab会不匹配
    tabContainer_->resetTabIndex();

    removeObsoleteLauncher();
}

void MainFrame::buildMenuBar()
{
    fileMenu_ = new QMenu(tr("file"), this);
    fileMenu_->setWindowFlags(fileMenu_->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    fileMenu_->setAttribute(Qt::WA_TranslucentBackground);

    auto action = new QAction(tr("new"));
    fileMenu_->addAction(action);
    connect(action, &QAction::triggered, this, &MainFrame::create);

    action = new QAction(tr("open"));
    fileMenu_->addAction(action);
    connect(action, &QAction::triggered, this, &MainFrame::open);

    exportAction_ = new QAction(tr("export"));
    fileMenu_->addAction(exportAction_);
    connect(exportAction_, &QAction::triggered, this, &MainFrame::saveTo);
    exportAction_->setEnabled(false);

    saveAction_ = new QAction(tr("save"));
    fileMenu_->addAction(saveAction_);
    connect(saveAction_, &QAction::triggered, this, &MainFrame::saveWithToast);
    saveAction_->setEnabled(false);

    saveAllAction_ = new QAction(tr("save all"));
    fileMenu_->addAction(saveAllAction_);
    connect(saveAllAction_, &QAction::triggered, this, &MainFrame::saveAllWithToast);
    saveAllAction_->setEnabled(false);

    helpMenu_ = new QMenu(tr("help"), this);
    helpMenu_->setWindowFlags(helpMenu_->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    helpMenu_->setAttribute(Qt::WA_TranslucentBackground);

    action = new QAction(tr("plugin manager"));
    connect(action, &QAction::triggered, this, [this]() {
        QPoint pos;
        pos.setX((width() - pluginManager_->width()) / 2);
        pos.setY(0);
        pluginManager_->move(mapToGlobal(pos));
        pluginManager_->show();
    });
    action->setVisible(false); //暂不支持
    helpMenu_->addAction(action);

    action = new QAction(tr("about"));
    helpMenu_->addAction(action);
    connect(action, &QAction::triggered, this, &MainFrame::onAboutTriggered);

#if 0
    QString languageName = "cn";
    menu = new QMenu(tr("upgrade test"), this);
    menuBar->addMenu(menu);

    action = new QAction(tr("requestModulesInfo"));
    connect(action, &QAction::triggered, this, [this, languageName]() {
        Upgrade::instance()->requestModulesInfo(languageName);
    });
    menu->addAction(action);

    action = new QAction(tr("requestGroupUpgradable"));
    connect(action, &QAction::triggered, this, [this, languageName]() {
        QMap<QString, QString> modulesName2version;
        modulesName2version.insert("mainFrame", "v1.0.0.0");
        modulesName2version.insert("aiboxPlugin", "v1.0.0.0");
        Upgrade::instance()->requestGroupUpgradable(languageName, modulesName2version);
    });
    menu->addAction(action);

    action = new QAction(tr("requestUpgradeFeedback"));
    connect(action, &QAction::triggered, this, [this]() {
        Upgrade::instance()->requestUpgradeFeedback();
    });
    menu->addAction(action);
#endif
}

void MainFrame::createNew(const QString &platform)
{
    QString classroomFolderPath = Settings::defaultClassroomFileAbsDir();
    QDir classroomFolder(classroomFolderPath);
    if (!classroomFolder.exists()) {
        classroomFolder.mkpath(".");
    }
    do {
        if (!classroomFileIndex.contains(platform)) {
            classroomFileIndex.insert(platform, 0);
        }
        QString classroomFilePath = QString("%1_%2").arg(platform).arg(classroomFileIndex[platform]++);
        classroomFilePath = FileDirHandler::absolutePath(classroomFilePath, classroomFolderPath);
        QFileInfo classroomFileInfo(classroomFilePath);
        if (!classroomFileInfo.exists()) {
            auto classroomWidget = pluginManager_->createNewClassroom(platform);
            classroomContainer_->addClassroom(classroomWidget, classroomFilePath);
            save();
            break;
        }
    } while(true);
}

void MainFrame::create()
{
    PlatformSelectionDialog* dialog = new PlatformSelectionDialog(pluginManager_, this);
    connect(dialog, &PlatformSelectionDialog::selection, this, &MainFrame::createNew);
//    dialog->move(geometry().topLeft());
    dialog->show();
}

void MainFrame::open()
{
    QString classroomFolderPath = Settings::defaultClassroomFileAbsDir();
    QDir classroomFolder(classroomFolderPath);
    if (!classroomFolder.exists()) {
        classroomFolder.mkpath(".");
    }
    QString classroomFilePath = QFileDialog::getOpenFileName(this, tr("select classroom file"), classroomFolderPath);
    QFileInfo classroomFileInfo(classroomFilePath);
    if (classroomFileInfo.exists()) {

        QSet<QString> allDevicesSNsInFile;
        QFile classroomFile(classroomFileInfo.absoluteFilePath());
        if (classroomFile.open(QFile::ReadOnly)) {
            QByteArray classroomData = classroomFile.readAll();
            classroomFile.close();
            auto jsonDocument = QJsonDocument::fromJson(classroomData);
            QJsonArray devicesArray = jsonDocument[ClassroomFileKey::Devices].toArray();
            for (auto jsonValue : devicesArray) {
                auto deviceJsonObj = jsonValue.toObject();
                allDevicesSNsInFile.insert(deviceJsonObj[ClassroomFileKey::DeviceSN].toString());
            }
        }
        QStringList errorStr;
        for (int i = 0; i < classroomContainer_->count(); ++i) {
            if (auto openedClassroomWidget = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(i))) {
                auto intersectDevices = openedClassroomWidget->allDevicesSNs().intersect(allDevicesSNsInFile);
                if (intersectDevices.count() != 0) {
                    for (auto intersectDevice : intersectDevices) {
                        errorStr.append(tr("device %1 has been added to %2 already").arg(intersectDevice).arg(classroomContainer_->tabText(i)));
                    }
                }
            }
        }
        if (errorStr.count() != 0) {
            errorStr.prepend(tr("open classroom file failed, device in file already added in opened classroom:"));
            LOG(INFO) << errorStr.join("\n").toStdString();
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::OnlyOkButton, this);
            warningwidget *warning = new warningwidget(errorStr.join("\n"), dialog);
            dialog->setDisplayWidget(warning);
            dialog->exec();
            return;
        }

        if (auto classroomWidget = pluginManager_->load(classroomFileInfo.absoluteFilePath())) {
            classroomContainer_->addClassroom(classroomWidget, classroomFileInfo.absoluteFilePath());
        }
    }
}

void MainFrame::save()
{
    if (auto devPluginUI = dynamic_cast<DevPluginUI*>(classroomContainer_->currentWidget())) {
        QString classroomFolderPath = classroomContainer_->getClassroomFilePath(classroomContainer_->currentIndex());
        if (classroomFolderPath.isEmpty()) {
            classroomFolderPath = Settings::defaultClassroomFileAbsDir();
            QDir classroomFolder(classroomFolderPath);
            if (!classroomFolder.exists()) {
                classroomFolder.mkpath(".");
            }
        }
        auto filePath = devPluginUI->save(classroomFolderPath);
        if (!filePath.isEmpty()) {
            QFileInfo fileInfo(filePath);
            classroomContainer_->setClassroomPath(classroomContainer_->currentIndex(), fileInfo.absolutePath());
        }
    }
}

void MainFrame::saveTo()
{
    if (auto devPluginUI = dynamic_cast<DevPluginUI*>(classroomContainer_->currentWidget())) {
        QString classroomFilePath = classroomContainer_->getClassroomFilePath(classroomContainer_->currentIndex());
        QString classroomFolderPath;
        if (classroomFilePath.isEmpty()) {
            classroomFolderPath = Settings::defaultClassroomFileAbsDir();
            QDir classroomFolder(classroomFolderPath);
            if (!classroomFolder.exists()) {
                classroomFolder.mkpath(".");
            }
            classroomFilePath = FileDirHandler::absolutePath(classroomContainer_->getClassroomName(classroomContainer_->currentIndex()), classroomFolderPath);
        }
        QFileInfo classroomFile(classroomFilePath);
        exportwidget *exportwd = new exportwidget(classroomFile.absolutePath());
        connect(exportwd, &exportwidget::sigExport, this, &MainFrame::saveToPath);
        exportwd->show();

    }
}

void MainFrame::saveToPath(QString path, QWidget *self)
{
    if (auto devPluginUI = dynamic_cast<DevPluginUI*>(classroomContainer_->currentWidget()))
    {
        QString classroomFilePath = FileDirHandler::absolutePath(classroomContainer_->getClassroomName(classroomContainer_->currentIndex()), path);
        classroomFilePath = devPluginUI->save(classroomFilePath);
        if (!classroomFilePath.isEmpty()) {
            classroomContainer_->setClassroomPath(classroomContainer_->currentIndex(), path);
            self->close();
        }
        showToast(tr("Export successfully!"));
    }
}

void MainFrame::saveAll()
{
    for (int i = 0; i <= classroomContainer_->count(); ++i) {
        if (auto devPluginUI = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(i))) {
            QString fileFolderPath = classroomContainer_->getClassroomFilePath(i);
            if (fileFolderPath.isEmpty()) {
                fileFolderPath = Settings::defaultClassroomFileAbsDir();
                QDir folder(fileFolderPath);
                if (!folder.exists()) {
                    folder.mkpath(".");
                }
            }
            auto filePath = devPluginUI->save(fileFolderPath);
            if (!filePath.isEmpty()) {
                QFileInfo fileInfo(filePath);
                classroomContainer_->setClassroomPath(i, fileInfo.absolutePath());
            }
        }
    }
}

void MainFrame::onNewTabRequest()
{
    PlatformSelectionDialog* dialog = new PlatformSelectionDialog(pluginManager_, this);
    connect(dialog, &PlatformSelectionDialog::selection, this, &MainFrame::createNew);
//    dialog->move(geometry().topLeft());
    dialog->show();
}

int tempindex;
void MainFrame::onRenameRequest(int index)
{
    tempindex = index;
    if (auto devPluginUI = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(index))) {
//        QString newName = QInputDialog::getText(this, tr("rename"), tr("new name:"));
        renamewidget *rename = new renamewidget();
        connect(rename, &renamewidget::sigRename, this, &MainFrame::onRename);
        rename->show();
    }
}

void MainFrame::onRename(QString name)
{
    QString classroomFilePath = classroomContainer_->getClassroomFilePath(tempindex);
    QString newClassroomFilePath = FileDirHandler::absolutePath(name, classroomContainer_->getClassroomPath(classroomContainer_->currentIndex()));
    QFileInfo oldeClassroomFileInfo(classroomFilePath);
    auto currentDir = oldeClassroomFileInfo.absoluteDir();
    const auto entryList = currentDir.entryList(QDir::Files);
    QString oldName = oldeClassroomFileInfo.fileName();
    if(oldName == name) {
        return;
    }
    if(entryList.contains(name)) {
        NameConflictDialog* dialog = new NameConflictDialog(this);
        dialog->setWarningMsg(tr("%1 is already exist in %2, please change a name").arg(name).arg(currentDir.absolutePath()));
        dialog->show();
        return;
    }

    if (oldeClassroomFileInfo.exists()) {
        if (QFile::copy(classroomFilePath, newClassroomFilePath)) {
            classroomContainer_->setClassroomName(tempindex, name);
            QFile::remove(classroomFilePath);
        }
    }
    else {
        newClassroomFilePath = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(tempindex))->save(newClassroomFilePath);
        if (!newClassroomFilePath.isEmpty()) {
            classroomContainer_->setClassroomName(tempindex, name);
            QFileInfo newClassroomFileInfo(newClassroomFilePath);
            classroomContainer_->setClassroomPath(tempindex, newClassroomFileInfo.absolutePath());
        }
    }
    tabContainer_->renameTab(tempindex,name);
}

void MainFrame::onCloseRequest(int index)
{
    if (auto devPluginUI = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(index))) {
        QString classroomFolderPath = classroomContainer_->getClassroomFilePath(index);
        if (classroomFolderPath.isEmpty()) {
            classroomFolderPath = Settings::defaultClassroomFileAbsDir();
            QDir classroomFolder(classroomFolderPath);
            if (!classroomFolder.exists()) {
                classroomFolder.mkpath(".");
            }
        }
        auto filePath = devPluginUI->save(classroomFolderPath);
        if (!filePath.isEmpty()) {
            QFileInfo fileInfo(filePath);
            classroomContainer_->setClassroomPath(index, fileInfo.absolutePath());
        }
        classroomContainer_->removeTab(index);
        classroomContainer_->setCurrentIndex(index - 1);
        devPluginUI->deleteLater();
    }
    if(classroomContainer_->count() == 1) {
        qDebug() << "no tab";
        exportAction_->setEnabled(false);
        saveAction_->setEnabled(false);
        saveAllAction_->setEnabled(false);
    }
}

void MainFrame::onCurrentClassroomChanged(int index)
{
    for (int i = 0; i < classroomContainer_->count(); ++i) {
        if (auto classroom = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(i))) {
            if (i == index) {
                classroom->activate();
            } else {
                classroom->deactivate();
            }
        }
    }
}

void MainFrame::onAboutTriggered()
{
//    aboutwidget *about = new aboutwidget(MainFrameVersion);
//    about->show();

    CommonDialog *dialog = new CommonDialog(tr("about"), CommonDialog::NoButton, this);
    auto aboutDlg = new AboutDlg(MainFrameVersion);
    dialog->setDisplayWidget(aboutDlg);
    dialog->exec();
}

void MainFrame::closeEvent(QCloseEvent *e)
{
    int widgetCount = classroomContainer_->count();
    bool deviceExecuting = false;
    for(int i = 0; i < widgetCount; ++i) {
        if(DevPluginUI* classroom = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(i))) {
            if(classroom->deviceExecuting()) {
                deviceExecuting = true;
                break;
            }
        }
    }
    if(deviceExecuting) {
        CloseWarningDialog* dialog = new CloseWarningDialog(this);
        connect(dialog,&CloseWarningDialog::sigAccepted,[this,e](bool accepted){
            if(accepted) {
                saveOpenedClassrooms();
                QMainWindow::closeEvent(e);
            } else {
                e->ignore();
            }
        });
        dialog->exec();
    } else {
        saveOpenedClassrooms();
        QMainWindow::closeEvent(e);
    }
}

void MainFrame::saveOpenedClassrooms()
{
    saveAll();

    QStringList files;
    for (int i = 0; i <= classroomContainer_->count(); ++i) {
        if (auto w = dynamic_cast<DevPluginUI*>(classroomContainer_->widget(i))) {
            QString classroomFilePath = classroomContainer_->getClassroomFilePath(i);
            if (!classroomFilePath.isEmpty()) {
                files.append(classroomFilePath);
            }
        }
    }
    Settings::saveOpenedClassroomsRecord(files);
}

void MainFrame::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton &&
            titleBar_->rect().contains(event->pos())){
        lBtnPressedPos_ = event->globalPos();
        lBtnPressed_ = true;
    }
}

void MainFrame::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && lBtnPressed_){
        lBtnPressed_ = false;
    }
}

void MainFrame::removeObsoleteLauncher()
{
    QDir launcherDir("../../launcher");
    auto launcherVersion = Config::laucherInfo();
    auto subDirsInfo = launcherDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (auto subDirInfo : subDirsInfo) {
        QDir subDir(subDirInfo.absoluteFilePath());
        if (subDir.dirName() != launcherVersion) {
            if (!subDir.removeRecursively()) {
                LOG(ERROR) << "remove dir " << subDir.absolutePath().toStdString() << " failed";
            }
        }
    }
}

void MainFrame::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons().testFlag(Qt::LeftButton) &&  lBtnPressed_){
        QPoint position = pos() + event->globalPos() - lBtnPressedPos_;
        move(position.x(), position.y());
        lBtnPressedPos_ = event->globalPos();
    }
}

void MainFrame::onMax()
{
    if(maximized_){
        maximized_ = false;
        this->showNormal();
    } else {
        maximized_ = true;
        this->showMaximized();
    }
}

void MainFrame::onMin()
{
    this->showMinimized();
}

void MainFrame::onClose()
{
    close();
}

void MainFrame::onFileTrigger()
{
    auto pos = mapToGlobal(titleBar_->getFileBtnBottomLeft());
    pos.setY(pos.y() + 10);
    fileMenu_->move(pos);
    fileMenu_->show();
}

void MainFrame::onHelpTrigger()
{
    auto pos = mapToGlobal(titleBar_->getHelpBtnBottomLeft());
    pos.setY(pos.y() + 10);
    helpMenu_->move(pos);
    helpMenu_->show();
}

void MainFrame::setLanguage(Language lan)
{
    LOG(INFO) << "MainFrame::setLanguage lan = " << lan << " current path:" << QDir::currentPath().toStdString();
    if (lan_ != lan) {
        lan_ = lan;
        while (translators_.count() != 0) {
            auto translator = translators_.takeFirst();
            qApp->removeTranslator(translator);
            translator->deleteLater();
            translator = nullptr;
        }
        switch(lan) {
        case Cn: {
            auto translator = new QTranslator(this);
            if (translator->load("language/mainFrame_cn") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load mainFrame_cn succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load mainFrame_cn failed";
                translator->deleteLater();
                translator = nullptr;
            }
            translator = new QTranslator(this);
            if (translator->load("language/utils") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load utils succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load utils failed";
                translator->deleteLater();
                translator = nullptr;
            }
        }
            break;
        default: {
            auto translator = new QTranslator(this);
            if (translator->load("language/mainFrame_cn") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load mainFrame_cn succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load mainFrame_cn failed";
                translator->deleteLater();
                translator = nullptr;
            }
            translator = new QTranslator(this);
            if (translator->load("language/utils") && qApp->installTranslator(translator)) {
                LOG(WARNING) << "load utils succeed";
                translators_.append(translator);
            } else {
                LOG(WARNING) << "load utils failed";
                translator->deleteLater();
                translator = nullptr;
            }
        }
            break;
        }
        if (pluginManager_) {
            pluginManager_->setLanguage(lan);
        }
    }
}

void MainFrame::onTabAdded(QString name)
{
    if(classroomContainer_->count() > 1) {
        qDebug() << "have tab";
        exportAction_->setEnabled(true);
        saveAction_->setEnabled(true);
        saveAllAction_->setEnabled(true);
    }
    tabContainer_->addTab(name);
}

void MainFrame::saveAllWithToast()
{
    saveAll();
    showToast(tr("Save successfully!"));
}

void MainFrame::saveWithToast()
{
    save();
    showToast(tr("Save successfully!"));
}

void MainFrame::showToast(const QString& text)
{
    auto dialog = new ToastDialog(this);
    dialog->setDisplayText(text);
    dialog->move((width()-dialog->width())/2,(height()-dialog->height())/2);
    dialog->show();
}

void MainFrame::onTabClicked(int index)
{
    classroomContainer_->setCurrentIndex(index);
}
