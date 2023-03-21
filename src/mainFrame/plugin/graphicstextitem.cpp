#include "graphicstextitem.h"

#include <QTextCursor>
#include <QCursor>
#include <QDebug>
#include <QFont>

#include "commondialog.h"
#include "warningwidget.h"

GraphicsTextItem::GraphicsTextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent)
{
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setDefaultTextColor(qRgba(0xff,0xff,0xff,0xcc));
    QFont font("Microsoft YaHei");
    font.setPixelSize(12);
    setFont(font);
    QRegExp regx("^[a-zA-Z0-9_\u4e00-\u9fa5]+$");
    validator_ = new QRegExpValidator(regx,this);
}

void GraphicsTextItem::setDeviceName(const QString& deviceName, bool isSn)
{
    deviceName_ = deviceName;
    if (isSn) {
        setPlainText(deviceName_.right(4));
    } else {
        setPlainText(deviceName_.left(4));
    }
}

void GraphicsTextItem::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    setPlainText(deviceName_);
    setCursor(Qt::IBeamCursor);
    auto cur = textCursor();
    qDebug() << "1:" /*<< cur*/;
    cur.clearSelection();
    cur.movePosition(QTextCursor::End);
    setTextCursor(cur);
}

void GraphicsTextItem::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    QString text = toPlainText();
    if (text != deviceName_) {
        int pos = 0;
        auto state = validator_->validate(text,pos);
        if(state != QValidator::Acceptable || containsInvalidCharacters(text) || text.size() > 16) {
            warningwidget *warning = new warningwidget(tr("Please enter Chinese characters, English letters, numbers or underscores, and less than 16 characthers"));
            CommonDialog *dialog = new CommonDialog(tr("system prompt"), CommonDialog::NoButton);
            dialog->setMinimumSize(580,170);
            dialog->setDisplayWidget(warning);
            dialog->show();
        } else if(!text.isEmpty() && text.size() <= 16) {
            deviceName_ = text;
        }
    }
    setPlainText(deviceName_.left(4));
    unsetCursor();
    update();
}

bool GraphicsTextItem::containsInvalidCharacters(const QString& s)
{
    if(s.contains(QString::fromLocal8Bit("【")) ||
            s.contains(QString::fromLocal8Bit("【")) ||
            s.contains(QString::fromLocal8Bit("】")) ||
            s.contains(QString::fromLocal8Bit("？")) ||
            s.contains(QString::fromLocal8Bit("！")) ||
            s.contains(QString::fromLocal8Bit("·")) ||
            s.contains(QString::fromLocal8Bit("￥")) ||
            s.contains(QString::fromLocal8Bit("……")) ||
            s.contains(QString::fromLocal8Bit("（")) ||
            s.contains(QString::fromLocal8Bit("）")) ||
            s.contains(QString::fromLocal8Bit("——")) ||
            s.contains(QString::fromLocal8Bit("、")) ||
            s.contains(QString::fromLocal8Bit("：")) ||
            s.contains(QString::fromLocal8Bit("；")) ||
            s.contains(QString::fromLocal8Bit("“")) ||
            s.contains(QString::fromLocal8Bit("”")) ||
            s.contains(QString::fromLocal8Bit("’")) ||
            s.contains(QString::fromLocal8Bit("‘")) ||
            s.contains(QString::fromLocal8Bit("《")) ||
            s.contains(QString::fromLocal8Bit("》")) ||
            s.contains(QString::fromLocal8Bit("，")) ||
            s.contains(QString::fromLocal8Bit("。"))) {
        return true;
    }
    return false;
}
