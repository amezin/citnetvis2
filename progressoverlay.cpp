#include "progressoverlay.h"

#include <QResizeEvent>
#include <QPoint>
#include <QDebug>

ProgressOverlay::ProgressOverlay(QWidget *parent) :
    QWidget(parent), prevAnimationDirection(QAbstractAnimation::Backward)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);

    bar = new QProgressBar(this);
    bar->setTextVisible(true);
    hide();

    effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);
    effect->setOpacity(0.0);

    animation = new QPropertyAnimation(effect, "opacity", effect);
    animation->setDuration(1000);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    connect(animation, SIGNAL(valueChanged(QVariant)),
            SLOT(animating(QVariant)));
}

void ProgressOverlay::setProgress(int value, int total)
{
    animate(QAbstractAnimation::Forward);

    bar->setValue(value);
    bar->setMaximum(total);
}

void ProgressOverlay::done()
{
    animate(QAbstractAnimation::Backward);
}

void ProgressOverlay::animating(const QVariant &value)
{
    if (value.toDouble() > 0.0) {
        show();
    } else {
        hide();
    }
}

void ProgressOverlay::animationDone()
{
    animating(QVariant(effect->opacity()));
}

void ProgressOverlay::animate(QAbstractAnimation::Direction direction)
{
    if (prevAnimationDirection == direction) {
        return;
    }

    animation->setDirection(direction);
    animation->start();
    prevAnimationDirection = direction;
}

void ProgressOverlay::resizeEvent(QResizeEvent *e)
{
    QSize barSize(e->size() * 0.5);
    barSize.setHeight(bar->sizeHint().height());
    QPoint topLeft((e->size().width() - barSize.width()) / 2,
                   (e->size().height() - barSize.height()) / 2);
    QRect barRect(topLeft, barSize);
    bar->setGeometry(barRect);

    QWidget::resizeEvent(e);
}
