#ifndef DOCKBUTTON_H
#define DOCKBUTTON_H

#include <QToolButton>
#include <QDockWidget>

class DockButton : public QToolButton
{
    Q_OBJECT
public:
    explicit DockButton(QAction *defaultAction, QWidget *parent = 0);

    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void changeEvent(QEvent *event);
};

#endif // DOCKBUTTON_H
