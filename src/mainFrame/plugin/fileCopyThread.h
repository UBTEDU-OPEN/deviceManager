#ifndef FILECOPYTHREAD_H
#define FILECOPYTHREAD_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>

class FileCopyThread : public QThread
{
    Q_OBJECT

public:
    explicit FileCopyThread(QObject *parent = nullptr);

    void setSrcFiles(const QStringList &srcFilesPathes) { srcFilesPathes_ = srcFilesPathes; }
    void setDstPath(const QString &dstDirPath) { dstDirPath_ = dstDirPath; }
    void onCancelRequest();

    void run() override;

signals:
    void copyStart();
    void copyFinished(QStringList failedFiles);
    void copyProgressing(qint64 p);

private:
    QStringList srcFilesPathes_;
    QString dstDirPath_;

    bool cancelRequested_;
};

#endif // FILECOPYTHREAD_H
