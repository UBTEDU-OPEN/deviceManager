#ifndef CLASSROOMCONTAINERTABBAR_H
#define CLASSROOMCONTAINERTABBAR_H

#include <QTabBar>

class QMouseEvent;
class QPaintEvent;
class QPushButton;

class ClassroomContainerTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit ClassroomContainerTabBar(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *me) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *re) override;

signals:
    void newTabRequest();
    void renameRequest(int index);
    void buttonClicked(int);

private:
    QRect tabBtnRect(int);
    bool tabBtnHover(int);

private:
    QPoint pos_;
    int hoverIndex_;

//    QPushButton lBtn_;
//    QPushButton rBtn_;
};

#endif // CLASSROOMCONTAINERTABBAR_H
