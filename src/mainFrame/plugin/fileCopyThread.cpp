#include "fileCopyThread.h"

#include "logHelper.h"
#include "fileDirHandler.h"
#include "md5.h"

#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QDir>

#include <QDebug>

FileCopyThread::FileCopyThread(QObject *parent)
    : QThread(parent)
    , cancelRequested_(false)
{
}

void FileCopyThread::onCancelRequest()
{
    cancelRequested_ = true;
}

void FileCopyThread::run()
{
    LOG(INFO) << "copy start:";
    LOG(INFO) << "source files(" << srcFilesPathes_.count() << "):" << srcFilesPathes_.join(" ").toStdString();
    LOG(INFO) << "destination files:" << dstDirPath_.toStdString();

    cancelRequested_ = false;
    qint64 totalSize = 0;
    qint64 totalCpySize = 0;
    for (auto &srcFilePath : srcFilesPathes_) {
        QFileInfo fileInfo(srcFilePath);
        LOG(INFO) << "file:" << fileInfo.baseName().toStdString();
        LOG(INFO) << "\tpath:" << fileInfo.absolutePath().toStdString();
        LOG(INFO) << "\tsize:" << fileInfo.size();
        totalSize += fileInfo.size();
    }

    emit copyStart();

    QDir destFileDir(dstDirPath_);
    QStringList failedFiles;
    for (QString &srcFilePath : srcFilesPathes_) {
        QFile srcfile(srcFilePath);
        if (!srcfile.open(QFile::ReadOnly)) {
            LOG(ERROR) << "open " << srcFilePath.toStdString() << " failed!";
            failedFiles.append(srcFilePath);
            continue;
        }
        QDataStream is(&srcfile);
        QFileInfo srcFileInfo(srcFilePath);
        const QString uploadingSufix = ".uploading";
        QString destFilePath = destFileDir.absoluteFilePath(srcFileInfo.fileName());

        QFileInfo destFileInfo(destFilePath);
        QString destFileBaseName = destFileInfo.baseName();
        int idx = 1;
        QString newDestFilePath = destFilePath;
        while (destFileInfo.exists()) {
            newDestFilePath = destFilePath;
            newDestFilePath.replace(destFileBaseName, destFileBaseName + QString("-copy(%1)").arg(idx));
            destFileInfo = QFileInfo(newDestFilePath);
            ++idx;
        }
        destFilePath = newDestFilePath;

        QFile destTempFile(destFilePath + uploadingSufix);
        if (!destTempFile.open(QFile::WriteOnly)) {
            LOG(ERROR) << "open " << srcFilePath.toStdString() << " failed!";
            failedFiles.append(srcFilePath);
            continue;
        }
        QDataStream os(&destTempFile);
        char* byteTemp = new char[4096];
        while (!is.atEnd()) {
            if (cancelRequested_) {
                break;
            }
            int readSize = 0;
            readSize = is.readRawData(byteTemp, 4096);
            os.writeRawData(byteTemp, readSize);
            totalCpySize += readSize;
            emit copyProgressing(100 * double(totalCpySize)/totalSize);
        }
        delete byteTemp;
        srcfile.close();
        destTempFile.close();

        QFileInfo destTempFileInfo(destFilePath + uploadingSufix);
        if (srcFileInfo.size() != destTempFileInfo.size() /*|| MD5::fileMd5(srcFilePath) != MD5::fileMd5(destFilePath + uploadingSufix)*/) {
            failedFiles.append(srcFilePath);
            QFile destTempFile(destFilePath + uploadingSufix);
            destTempFile.remove();
        } else {
            QFile destFile(destFilePath + uploadingSufix);
            if (destFile.open(QFile::ReadWrite)) {
                destFile.rename(destFilePath);
                destFile.close();
            }
        }
    }

    for (QFileInfo fileInfo : destFileDir.entryInfoList({"*.uploading"}, QDir::Files)) {
        QFile tempFile(fileInfo.absoluteFilePath());
        if (tempFile.exists()) {
            tempFile.remove();
        }
    }
    emit copyFinished(failedFiles);
}

