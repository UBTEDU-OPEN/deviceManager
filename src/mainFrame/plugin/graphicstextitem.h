#ifndef GRAPHICSTEXTITEM_H
#define GRAPHICSTEXTITEM_H

#include <QGraphicsTextItem>
#include <QRegExpValidator>

class GraphicsTextItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    GraphicsTextItem(QGraphicsItem *parent = nullptr);
    void setDeviceName(const QString& deviceName, bool isSn);
    QString deviceName() { return deviceName_; }

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    bool containsInvalidCharacters(const QString&);

private:
    QString deviceName_;
    QRegExpValidator *validator_;
};

#endif // GRAPHICSTEXTITEM_H
