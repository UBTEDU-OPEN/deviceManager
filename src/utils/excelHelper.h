#ifndef EXCELHELPER_H
#define EXCELHELPER_H

#include "utilsGlobal.h"

#include <QList>
#include <QPair>

class UTILS_EXPORT ExcelHelper
{
public:
    ExcelHelper() = delete;

    static QList<QPair<QString, QString>> readSnAndIdFromExcel(const QString &filePath);
};

#endif // EXCELHELPER_H
