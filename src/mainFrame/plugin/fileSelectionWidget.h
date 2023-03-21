#ifndef FILESELECTIONWIDGET_H
#define FILESELECTIONWIDGET_H

#include <QWidget>

namespace Ui {
class FileSelectionWidget;
}

class FileSelectionWidget : public QWidget
{
    Q_OBJECT

private:
    static const QString QSS_PATH;

public:
    explicit FileSelectionWidget(QWidget *parent = nullptr);
    ~FileSelectionWidget();

    void onSelectClicked();
    void onUploadClicked(bool accepted);

protected:
    void setDefaultStyle();

signals:
    void uploadFileSelecteChanged(bool selected);
    void closeRequest();
    void fileUploaded();

private:
    Ui::FileSelectionWidget *ui;

    QString filePath_;
};

#endif // FILESELECTIONWIDGET_H
