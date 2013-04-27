#ifndef PERSISTENTFIELD_H
#define PERSISTENTFIELD_H

#include <QLineEdit>

#include "persistentwidget.h"

class PersistentField : public QLineEdit, public PersistentWidget
{
    Q_OBJECT
public:
    PersistentField(const QString &name, const QString &value,
                    QWidget *parent = 0);

    virtual void loadState(const QSettings *);
    virtual void saveState(QSettings *) const;

};

#endif // PERSISTENTFIELD_H
