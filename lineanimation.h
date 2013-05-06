#ifndef LINEANIMATION_H
#define LINEANIMATION_H

#include <QVariantAnimation>
#include <QGraphicsLineItem>

class LineAnimation : public QVariantAnimation
{
    Q_OBJECT
public:
    LineAnimation(QGraphicsLineItem *, const QLineF &, QObject *parent = 0);
    
protected:
    virtual void updateCurrentValue(const QVariant &value);

private:
    QGraphicsLineItem *item;
};

#endif // LINEANIMATION_H
