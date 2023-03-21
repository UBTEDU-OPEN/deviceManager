#include "listview.h"

#include <QToolTip>
#include <QMouseEvent>
#include <QFileSystemModel>

ListView::ListView(QWidget *parent)
    : QListView(parent)
    , lastRow_(-1)
{
}


void ListView::mouseMoveEvent(QMouseEvent *e)
{
    auto index = indexAt(e->pos());
    auto fileSystemModel = dynamic_cast<QFileSystemModel*>(model());
    if(index.isValid() && lastIndex_ != index) {
        QToolTip::showText(e->globalPos(),fileSystemModel->fileInfo(index).fileName());
    }
    lastIndex_ = index;
}
