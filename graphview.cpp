#include "graphview.h"

#include <QGridLayout>
#include <QToolButton>
#include <QWheelEvent>
#include <QDebug>

#include <qmath.h>

GraphView::GraphView(QGraphicsScene *scene, QWidget *parent) :
    QWidget(parent)
{
    view = new QGraphicsView(scene, this);

    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setOptimizationFlag(QGraphicsView::DontSavePainterState);
    view->setDragMode(QGraphicsView::ScrollHandDrag);

    auto layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view, 0, 0, 2, 1);

    slider = new QSlider(Qt::Vertical, this);
    layout->addWidget(slider, 0, 1);
    slider->setRange(-100, 100);
    slider->setValue(0);
    connect(slider, SIGNAL(valueChanged(int)), SLOT(setScale()));

    auto resetScaleButton = new QToolButton(this);
    resetScaleButton->setIcon(QIcon::fromTheme("zoom-original"));
    connect(resetScaleButton, SIGNAL(clicked()), SLOT(resetScale()));
    layout->addWidget(resetScaleButton, 1, 1);

    overlay = new ProgressOverlay(this);

    setScale();
}

void GraphView::setScale()
{
    QTransform transform;
    static const qreal sliderScale = 100;
    transform.scale(qPow(10, slider->value() / sliderScale),
                    qPow(10, slider->value() / sliderScale));
    view->setTransform(transform);
}

void GraphView::resetScale()
{
    slider->setValue(0);
}

void GraphView::resizeEvent(QResizeEvent *event)
{
    overlay->setGeometry(0, 0, event->size().width(), event->size().height());

    QWidget::resizeEvent(event);
}
