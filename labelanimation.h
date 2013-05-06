#ifndef LABELANIMATION_H
#define LABELANIMATION_H

#include <QVariantAnimation>
#include <QGraphicsSimpleTextItem>

class LabelAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    LabelAnimation(QGraphicsSimpleTextItem *, const QPointF &,
                   QObject *parent = 0);

protected:
    virtual void updateCurrentValue(const QVariant &value);

private:
    QGraphicsSimpleTextItem *item;
};

#endif // LABELANIMATION_H
