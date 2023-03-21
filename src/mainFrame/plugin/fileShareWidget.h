#ifndef FILESHAREWIDGET_H
#define FILESHAREWIDGET_H

#include <QWidget>

class SharedFileListItemDelegate;

namespace Ui {
class FileShareWidget;
}

class FileShareWidget : public QWidget
{
    Q_OBJECT

private:
    static const QString QSS_PATH;

public:
    explicit FileShareWidget(QWidget *parent = nullptr);
    ~FileShareWidget();

    void onASddShareFileClicked();
    void onDeleteRequest(const QModelIndex index);

signals:
    void shareListChanged();

protected:
    void setDefaultStyle();

private:
    Ui::FileShareWidget *ui;

    SharedFileListItemDelegate *sharedFileListItemDelegate_;
};

#endif // FILESHAREWIDGET_H
