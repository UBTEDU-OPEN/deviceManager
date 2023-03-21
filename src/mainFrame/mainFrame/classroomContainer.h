#ifndef CLASSROOMCONTAINER_H
#define CLASSROOMCONTAINER_H

#include <QTabWidget>
#include <QString>
#include <QSet>

class DevPluginUI;
class QTcpSocket;

class ClassroomContainer : public QTabWidget
{
    Q_OBJECT
public:
    explicit ClassroomContainer(QWidget *parent = nullptr);

    int addClassroom(DevPluginUI *classroomUI, const QString &classroomFilePath, const QIcon &icon = QIcon());
    QString getClassroomFilePath(int index) const;
    QString getClassroomPath(int index) const;
    void setClassroomPath(int index, const QString &classroomFilePath);
    void setClassroomName(int index, const QString &name);
    QString getClassroomName(int index) const;

protected:
    void onActivateRequest();
    void onNewCmdConnection(QTcpSocket *tcpSocket);
    void onNewTransferConnection(QTcpSocket *tcpSocket);
    QSet<QString> allDevices();

signals:
    void newTabRequest();
    void renameRequest(int index);
    void closeRequest(int index);
    void tabAdded(QString);
};

#endif // CLASSROOMCONTAINER_H
