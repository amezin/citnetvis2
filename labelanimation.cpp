#include "labelanimation.h"

LabelAnimation::LabelAnimation(QGraphicsSimpleTextItem *item, const QPointF &p,
                               QObject *parent)
    : QVariantAnimation(parent), item(item)
{
    setStartValue(item->pos());
    setEndValue(p);
}

void LabelAnimation::updateCurrentValue(const QVariant &value)
{
    item->setPos(qvariant_cast<QPointF>(value));
}
