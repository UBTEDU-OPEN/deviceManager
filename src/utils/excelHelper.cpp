#include "excelHelper.h"

#include <QString>

#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "xlsxrichstring.h"
#include "xlsxworkbook.h"

#include "BasicExcel.hpp"

QList<QPair<QString, QString>> ExcelHelper::readSnAndIdFromExcel(const QString &filePath)
{
    QList<QPair<QString, QString>> sns2ids;
    if (filePath.endsWith(".xlsx")) {
        QXlsx::Document xlsxR(filePath);
        if (xlsxR.load()) {
            int row = 1;
            do {
                if (auto cell = xlsxR.cellAt(row, 1)) {
                    QPair<QString, QString> sn2id;
                    sn2id.first = cell->readValue().toString();
                    if (sn2id.first.isEmpty()) {
                        break;
                    }
                    if (cell = xlsxR.cellAt(row, 2)) {
                        sn2id.second = cell->readValue().toString();
                    }
                    ++row;
                    sns2ids.append(sn2id);
                    continue;
                }
                break;
            } while(true);
        }
    } else if (filePath.endsWith(".xls")) {
        YExcel::BasicExcel e;
        e.Load(filePath.toStdString().c_str());
        auto sheetsCount = e.GetTotalWorkSheets();
        if (sheetsCount > 0) {
            if (auto sheet1 = e.GetWorksheet(size_t(0))) {
                auto maxRows = sheet1->GetTotalRows();
                auto maxCols = sheet1->GetTotalCols();
                if (maxCols >= 1) {
                    for (size_t  r = 0; r < maxRows; ++r) {
                        QPair<QString, QString> sn2id;
                        for (size_t c = 0; c < maxCols; ++c) {
                            QString valStr;
                            auto cell = sheet1->Cell(r, c);
                            switch (cell->Type()) {
                            case YExcel::BasicExcelCell::UNDEFINED: {
                            }
                                break;
                            case YExcel::BasicExcelCell::INT: {
                                valStr = QString::number(cell->GetInteger());
                            }
                                break;
                            case YExcel::BasicExcelCell::DOUBLE: {
                                valStr = QString::number(cell->GetDouble());
                            }
                                break;
                            case YExcel::BasicExcelCell::STRING: {
                                valStr = QString::fromStdString(cell->GetString());
                            }
                                break;
                            case YExcel::BasicExcelCell::WSTRING: {
                                valStr = QString::fromStdWString(cell->GetWString());
                            }
                                break;
                            }
                            if (c == 0) {
                                sn2id.first = valStr;
                            } else if (c == 1 && !sn2id.first.isEmpty()) {
                                sn2id.second = valStr;
                                sns2ids.append(sn2id);
                            }
                        }
                    }
                }
            }
        }
    }
    return sns2ids;
}
