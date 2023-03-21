#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "devicePluginInterface.h"

#include <QMainWindow>
#include <QStringList>
#include <QList>

class PluginManager;
class ClassroomContainer;
class TitleBar;
class TabContainer;

class QMenu;
class QTranslator;
class QTabWidget;
class QCloseEvent;

class MainFrame : public QMainWindow
{
    Q_OBJECT

public:
    static QMap<QString, int> classroomFileIndex;

public:
    explicit MainFrame(QWidget *parent = nullptr);

public:
    void buildMenuBar();

    void createNew(const QString &platform);

    void create();
    void open();
    void save();
    void saveTo();
    void saveToPath(QString path, QWidget *self);
    void saveAll();
    void saveAllWithToast();
    void saveWithToast();
    void showToast(const QString& text);

    void onNewTabRequest();
    void onRenameRequest(int index);
    void onRename(QString name);
    void onCloseRequest(int index);
    void onCurrentClassroomChanged(int index);
    void onTabAdded(QString);
    void onTabClicked(int);

    void onAboutTriggered();
    void onClose();
    void onMax();
    void onMin();
    void onFileTrigger();
    void onHelpTrigger();

    void setLanguage(Language lan);

protected:
    void closeEvent(QCloseEvent *e) override;
    void saveOpenedClassrooms();
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void removeObsoleteLauncher();

private:
    PluginManager           *pluginManager_;
    QMenu                   *contextMenu_;
    QMenu                   *fileMenu_;
    QMenu                   *helpMenu_;
    QAction                 *exportAction_;
    QAction                 *saveAction_;
    QAction                 *saveAllAction_;
    ClassroomContainer      *classroomContainer_;
    TitleBar                *titleBar_;
    bool                    maximized_;
    bool                    lBtnPressed_;
    QPoint                  lBtnPressedPos_;

    Language                lan_;
    QList<QTranslator*>     translators_;

    TabContainer            *tabContainer_;
    QWidget                 *tabWidgetContainer_;
};

#endif // MAINFRAME_H
