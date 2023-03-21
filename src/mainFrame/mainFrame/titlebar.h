#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QMenu>

namespace Ui {
	class titlebar;
}

class TitleBar: public QWidget {
Q_OBJECT

public:
	explicit TitleBar(QWidget *parent = nullptr);
	~TitleBar();

    QPoint getFileBtnBottomLeft();
    QPoint getHelpBtnBottomLeft();
              
signals:
	void sigMin();
	void sigMax();
	void sigClose();
    void sigFileTrigger();
    void sigHelpTrigger();

private:
	Ui::titlebar *ui;
};

#endif // TITLEBAR_H
