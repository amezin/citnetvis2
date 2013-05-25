#include "graphview.h"

#include <QGridLayout>
#include <QToolButton>
#include <QAction>
#include <QDebug>
#include <QResizeEvent>

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
    layout->addWidget(view, 0, 0, 4, 1);

    slider = new QSlider(Qt::Vertical, this);
    layout->addWidget(slider, 1, 1);
    slider->setRange(-150, 50);
    slider->setValue(0);
    connect(slider, SIGNAL(valueChanged(int)), SLOT(setScale()));

    zoomOriginalAction = new QAction(QIcon::fromTheme("zoom-original"),
                                          "O&riginal zoom", this);
    connect(zoomOriginalAction, SIGNAL(triggered()), SLOT(zoomOriginal()));
    addAction(zoomOriginalAction);
    auto zoomOriginalButton = new QToolButton(this);
    zoomOriginalButton->setDefaultAction(zoomOriginalAction);
    layout->addWidget(zoomOriginalButton, 0, 1);

    zoomInAction = new QAction(QIcon::fromTheme("zoom-in"),
                                          "Zoom &in", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, SIGNAL(triggered()), SLOT(zoomIn()));
    addAction(zoomInAction);
    auto zoomInButton = new QToolButton(this);
    zoomInButton->setDefaultAction(zoomInAction);
    layout->addWidget(zoomInButton, 2, 1);

    zoomOutAction = new QAction(QIcon::fromTheme("zoom-out"),
                                          "Zoom &out", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, SIGNAL(triggered()), SLOT(zoomOut()));
    addAction(zoomOutAction);
    auto zoomOutButton = new QToolButton(this);
    zoomOutButton->setDefaultAction(zoomOutAction);
    layout->addWidget(zoomOutButton, 3, 1);

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

    zoomOutAction->setEnabled(slider->value() > slider->minimum());
    zoomInAction->setEnabled(slider->value() < slider->maximum());
    zoomOriginalAction->setEnabled(slider->value() != 0);
}

void GraphView::zoomOriginal()
{
    slider->setValue(0);
}

void GraphView::zoomIn()
{
    slider->setValue(slider->value() + 10);
}

void GraphView::zoomOut()
{
    slider->setValue(slider->value() - 10);
}

void GraphView::resizeEvent(QResizeEvent *event)
{
    overlay->setGeometry(0, 0, event->size().width(), event->size().height());

    QWidget::resizeEvent(event);
}
