#ifndef NODEANIMATION_H
#define NODEANIMATION_H

#include <QVariantAnimation>
#include <QGraphicsEllipseItem>

class NodeAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    NodeAnimation(QGraphicsEllipseItem *, const QRectF &, QObject *parent = 0);

protected:
    virtual void updateCurrentValue(const QVariant &value);

private:
    QGraphicsEllipseItem *item;
};

#endif // NODEANIMATION_H
