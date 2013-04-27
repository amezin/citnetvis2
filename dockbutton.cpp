#include "dockbutton.h"

#include <QDebug>
#include <QToolBar>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QMainWindow>
#include <QEvent>

DockButton::DockButton(QAction *defaultAction, QWidget *parent) :
    QToolButton(parent)
{
    Q_ASSERT(defaultAction);

    setDefaultAction(defaultAction);
}

static Qt::ToolBarArea toolBarArea(const DockButton *button)
{
    Q_ASSERT(button);
    auto bar = qobject_cast<QToolBar *>(button->parent());
    Q_ASSERT(bar);
    auto win = qobject_cast<QMainWindow *>(bar->parentWidget());
    Q_ASSERT(win);
    return win->toolBarArea(bar);
}

static Qt::Orientation toolBarOrientation(const DockButton *button)
{
    switch (toolBarArea(button)) {
    case Qt::LeftToolBarArea:
    case Qt::RightToolBarArea:
        return Qt::Vertical;
    default:
        return Qt::Horizontal;
    }
}

QSize DockButton::sizeHint() const
{
    QSize hint = QToolButton::sizeHint();
    if (toolBarOrientation(this) == Qt::Vertical) {
        hint.transpose();
    }
    return hint;
}

void DockButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStylePainter p(this);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    if (toolBarOrientation(this) == Qt::Vertical)
    {
        QSize size = opt.rect.size();
        size.transpose();
        opt.rect.setSize(size);

        if (toolBarArea(this) == Qt::RightToolBarArea) {
            p.rotate(90);
            p.translate(0, -width());
        } else {
            p.rotate(-90);
            p.translate(-height(), 0);
        }
    }

    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

void DockButton::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        if (!isEnabled()) {
            setEnabled(true);
        }
    }
    QToolButton::changeEvent(event);
}
