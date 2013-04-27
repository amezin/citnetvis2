#include "widgetwithoverlay.h"

#include <QGridLayout>
#include <QResizeEvent>

WidgetWithOverlay::WidgetWithOverlay(QWidget *widget, QWidget *overlay,
                                     QWidget *parent)
    : QWidget(parent)
{
    widget->setParent(this);
    this->widget = widget;

    overlay->setParent(this);
    this->overlay = overlay;
}

void WidgetWithOverlay::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    overlay->resize(e->size());
    widget->resize(e->size());
}
