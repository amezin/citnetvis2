#ifndef PROGRESSOVERLAY_H
#define PROGRESSOVERLAY_H

#include <QWidget>
#include <QProgressBar>
#include <QGraphicsEffect>
#include <QPropertyAnimation>

class ProgressOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressOverlay(QWidget *parent);

public slots:
    void setProgress(int value, int total);
    void done();

protected:
    virtual void resizeEvent(QResizeEvent *);

private slots:
    void animating(const QVariant &);
    void animationDone();

private:
    void animate(QAbstractAnimation::Direction);

    QProgressBar *bar;
    QGraphicsOpacityEffect *effect;
    QPropertyAnimation *animation;
    QAbstractAnimation::Direction prevAnimationDirection;
};

#endif // PROGRESSOVERLAY_H
