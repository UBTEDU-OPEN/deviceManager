#ifndef FILECOPYPROGRESSDLG_H
#define FILECOPYPROGRESSDLG_H

#include <QDialog>
#include <QStringList>
#include <QString>

class FileCopyThread;

namespace Ui {
class FileCopyProgressDlg;
}

class FileCopyProgressDlg : public QDialog
{
    Q_OBJECT

private:
    static const QString QSS_PATH;

public:
    explicit FileCopyProgressDlg(const QStringList &srcFiles, const QString &dstDir, QWidget *parent = nullptr);
    ~FileCopyProgressDlg();

    void onCancel();
    void onStart();
    void onCpyProgressing(qint64 value);
    void onFinish(QStringList failedFiles);

protected:
    void setDefaultStyle();

private:
    Ui::FileCopyProgressDlg *ui;

    FileCopyThread *fileCopyThread_;
};

#endif // FILECOPYPROGRESSDLG_H
