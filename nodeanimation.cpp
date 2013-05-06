#include "nodeanimation.h"

NodeAnimation::NodeAnimation(QGraphicsEllipseItem *item, const QRectF &target,
                             QObject *parent)
    : QVariantAnimation(parent), item(item)
{
    setStartValue(item->rect());
    setEndValue(target);
}

void NodeAnimation::updateCurrentValue(const QVariant &value)
{
    item->setRect(qvariant_cast<QRectF>(value));
}
