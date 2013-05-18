#include "persistentcheck.h"

PersistentCheck::PersistentCheck(const QString &name, QWidget *parent)
    : QCheckBox(parent)
{
    setObjectName(name);
}

void PersistentCheck::loadState(const QSettings *settings)
{
    QString myName(objectName());
    if (settings->contains(myName)) {
        setValue(settings->value(myName).toBool());
    }
}

void PersistentCheck::saveState(QSettings *settings) const
{
    settings->setValue(objectName(), value());
}

