#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QGraphicsView>
#include <QSlider>

#include "progressoverlay.h"

class GraphView : public QWidget
{
    Q_OBJECT
public:
    explicit GraphView(QGraphicsScene *, QWidget *parent = 0);

    ProgressOverlay *progressOverlay() const { return overlay; }

public slots:
    void zoomOriginal();
    void zoomIn();
    void zoomOut();

protected:
    virtual void resizeEvent(QResizeEvent *);

private slots:
    void setScale();

private:
    QGraphicsView *view;
    QSlider *slider;
    ProgressOverlay *overlay;
    QAction *zoomInAction, *zoomOutAction, *zoomOriginalAction;
};

#endif // GRAPHVIEW_H
