#include "lineanimation.h"

LineAnimation::LineAnimation(QGraphicsLineItem *item, const QLineF &target,
                             QObject *parent)
    : QVariantAnimation(parent), item(item)
{
    setStartValue(item->line());
    setEndValue(target);
}

void LineAnimation::updateCurrentValue(const QVariant &value)
{
    item->setLine(qvariant_cast<QLineF>(value));
}
