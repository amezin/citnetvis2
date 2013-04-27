#ifndef WIDGETWITHOVERLAY_H
#define WIDGETWITHOVERLAY_H

#include <QWidget>

class WidgetWithOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetWithOverlay(QWidget *widget, QWidget *overlay,
                               QWidget *parent = 0);

protected:
    virtual void resizeEvent(QResizeEvent *);

private:
    QWidget *widget, *overlay;
};

#endif // WIDGETWITHOVERLAY_H
