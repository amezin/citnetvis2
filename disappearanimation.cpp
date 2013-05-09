#include "disappearanimation.h"

DisappearAnimation::DisappearAnimation(const QSharedPointer<QGraphicsItem> &p,
                                       QObject *parent)
    : OpacityAnimation(p.data(), 0, parent), item(p)
{
}
