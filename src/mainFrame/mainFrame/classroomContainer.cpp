#include "classroomContainer.h"

#include "devicePluginInterface.h"
#include "classroomContainerTabBar.h"
#include "fileDirHandler.h"
#include "addNewClassroomWidget.h"

#include <QFileInfo>
#include <QIcon>
#include <QVariant>

ClassroomContainer::ClassroomContainer(QWidget *parent)
    : QTabWidget(parent)
{
    auto tabbar = new ClassroomContainerTabBar;
    setTabBar(tabbar);

    AddNewClassroomWidget *addNewClassroomWidget = new AddNewClassroomWidget;
    addTab(addNewClassroomWidget, "+");
    tabbar->setTabButton(0, QTabBar::RightSide, nullptr);
    connect(addNewClassroomWidget, &AddNewClassroomWidget::addNewClassroomRequest, this, &ClassroomContainer::newTabRequest);
    connect(tabbar, &ClassroomContainerTabBar::newTabRequest, this, &ClassroomContainer::newTabRequest);
    connect(tabbar, &ClassroomContainerTabBar::renameRequest, this, &ClassroomContainer::renameRequest);
    connect(tabbar, &ClassroomContainerTabBar::buttonClicked, this, &ClassroomContainer::closeRequest);
}

int ClassroomContainer::addClassroom(DevPluginUI *classroomUI, const QString &classroomFilePath, const QIcon &icon)
{
    QFileInfo classroomFileInfo(classroomFilePath);
    int res = -1;
    if (icon.isNull()) {
        res = insertTab(count() - 1, classroomUI, classroomFileInfo.baseName());
    } else {
        res = insertTab(count() - 1, classroomUI, icon, classroomFileInfo.baseName());
    }
    classroomUI->setContainer(this);
    tabBar()->setTabData(res, classroomFileInfo.absolutePath());
    tabBar()->setTabToolTip(res, classroomFilePath);
    setCurrentIndex(res);

    connect(classroomUI, &DevPluginUI::activateRequest, this, &ClassroomContainer::onActivateRequest);
    emit tabAdded(classroomFileInfo.baseName());
    return res;
}

QString ClassroomContainer::getClassroomFilePath(int index) const
{
    if (index >= 0 && index < count()) {
        return FileDirHandler::absolutePath(tabBar()->tabText(index), tabBar()->tabData(index).toString());
    }
    return "";
}

QString ClassroomContainer::getClassroomPath(int index) const
{
    if (index >= 0 && index < count()) {
        return tabBar()->tabData(index).toString();
    }
    return "";
}

void ClassroomContainer::setClassroomPath(int index, const QString &classroomPath)
{
    if (index >= 0 && index < count()) {
        tabBar()->setTabData(index, classroomPath);
        QString newTooltip = FileDirHandler::absolutePath(tabBar()->tabText(index), classroomPath);
        tabBar()->setTabToolTip(index, newTooltip);
    }
}

void ClassroomContainer::setClassroomName(int index, const QString &name)
{
    if (index >= 0 && index < count()) {
        tabBar()->setTabText(index, name);
        QString newTooltip = FileDirHandler::absolutePath(name, tabBar()->tabData(index).toString());
        tabBar()->setTabToolTip(index, newTooltip);
    }
}

QString ClassroomContainer::getClassroomName(int index) const
{
    if (index >= 0 && index < count()) {
        return tabBar()->tabText(index);
    }
    return "";
}

void ClassroomContainer::onActivateRequest()
{
    if (auto senderDevUI = dynamic_cast<DevPluginUI*>(sender())) {
//        for (int i = 0; i < count(); ++i) {
//            if (auto devUI = dynamic_cast<DevPluginUI*>(widget(i))) {
//                devUI->deactivate();
//            }
//        }
        senderDevUI->activate();
    }
}

