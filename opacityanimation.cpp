#include "opacityanimation.h"

OpacityAnimation::OpacityAnimation(QGraphicsItem *item, qreal target,
                                   QObject *parent)
    : QVariantAnimation(parent), item(item)
{
    setStartValue(item->opacity());
    setEndValue(target);
}

void OpacityAnimation::updateCurrentValue(const QVariant &value)
{
    item->setOpacity(qvariant_cast<qreal>(value));
}
