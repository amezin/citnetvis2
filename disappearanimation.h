#ifndef DISAPPEARANIMATION_H
#define DISAPPEARANIMATION_H

#include <QSharedPointer>

#include "opacityanimation.h"

class DisappearAnimation : public OpacityAnimation
{
    Q_OBJECT
public:
    DisappearAnimation(const QSharedPointer<QGraphicsItem> &,
                       QObject *parent = 0);

private:
    QSharedPointer<QGraphicsItem> item;
};

#endif // DISAPPEARANIMATION_H
