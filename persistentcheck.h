#ifndef PERSISTENTCHECK_H
#define PERSISTENTCHECK_H

#include <QCheckBox>

#include "persistentwidget.h"

class PersistentCheck : public QCheckBox, public PersistentWidget
{
    Q_OBJECT
public:
    PersistentCheck(const QString &name, QWidget *parent = 0);

    virtual void loadState(const QSettings *);
    virtual void saveState(QSettings *) const;

    void setValue(bool v) { setCheckState(v ? Qt::Checked : Qt::Unchecked); }
    bool value() const { return checkState() == Qt::Checked; }
};

#endif // PERSISTENTCHECK_H
