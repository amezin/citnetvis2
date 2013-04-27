#include "persistentfield.h"

PersistentField::PersistentField(const QString &name, const QString &value,
                                 QWidget *parent)
    : QLineEdit(value, parent)
{
    setObjectName(name);
}

void PersistentField::loadState(const QSettings *settings)
{
    QString myName = objectName();
    if (settings->contains(myName)) {
        setText(settings->value(myName).toString());
    }
}

void PersistentField::saveState(QSettings *settings) const
{
    QString myName = objectName();
    settings->setValue(myName, text());
}
