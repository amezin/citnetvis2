#ifndef OPACITYANIMATION_H
#define OPACITYANIMATION_H

#include <QVariantAnimation>
#include <QGraphicsItem>

class OpacityAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    explicit OpacityAnimation(QGraphicsItem *, qreal target,
                              QObject *parent = 0);

protected:
    virtual void updateCurrentValue(const QVariant &value);

private:
    QGraphicsItem *item;
};

#endif // OPACITYANIMATION_H
