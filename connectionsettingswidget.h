#ifndef CONNECTIONSETTINGSWIDGET_H
#define CONNECTIONSETTINGSWIDGET_H

#include <QWidget>

#include "persistentfield.h"

class ConnectionSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionSettingsWidget(QWidget *parent = 0);

    QString endpointUrl() const { return endpointUrlEdit->text(); }
    QString datePredicate() const { return dateEdit->text(); }
    QString dateRegEx() const { return dateRegExEdit->text(); }
    QString titlePredicate() const { return titleEdit->text(); }
    QString referencePredicate() const { return referenceEdit->text(); }

private:
    PersistentField *endpointUrlEdit, *dateEdit, *titleEdit, *referenceEdit,
    *dateRegExEdit;
};

#endif // CONNECTIONSETTINGSWIDGET_H
