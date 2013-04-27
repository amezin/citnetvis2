#ifndef PERSISTENTWIDGET_H
#define PERSISTENTWIDGET_H

#include <QSettings>

class PersistentWidget
{
public:
    virtual void saveState(QSettings *) const = 0;
    virtual void loadState(const QSettings *) = 0;
};

#endif // PERSISTENTWIDGET_H
