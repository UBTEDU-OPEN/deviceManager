#include "graphicsscene.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QTransform>

#include "commonDeviceItem.h"

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QTransform transform;
    QGraphicsItem* selectedItem = itemAt(event->scenePos().x(), event->scenePos().y(), transform);
    if (!selectedItem) {
        boxChoose_ = true;
        emit boxChooseBegin();
    }
    QGraphicsScene::mousePressEvent(event);
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    if (boxChoose_) {
        boxChoose_ = false;
        emit boxChooseEnd();
    }
}
