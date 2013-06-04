#ifndef DATASETTINGSWIDGET_H
#define DATASETTINGSWIDGET_H

#include <QWidget>

#include "persistentfield.h"
#include "persistentcheck.h"

class DataSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataSettingsWidget(QWidget *parent = 0);

    QString endpointUrl() const { return endpointUrlEdit->text(); }
    QString datePredicate() const { return dateEdit->text(); }
    QString dateRegEx() const { return dateRegExEdit->text(); }
    QString titlePredicate() const { return titleEdit->text(); }
    QString referencePredicate() const { return referenceEdit->text(); }
    bool loadReferences() const { return recursiveCheck->value(); }
    bool useBarycenterHeuristic() const { return barycenterCheck->value(); }
    bool useSlowAlgorithm() const { return slowCheck->value(); }
    bool randomize() const { return randomizeCheck->value(); }

private:
    PersistentField *endpointUrlEdit, *dateEdit, *titleEdit, *referenceEdit,
    *dateRegExEdit;
    PersistentCheck *recursiveCheck, *barycenterCheck, *slowCheck,
    *randomizeCheck;
};

#endif // DATASETTINGSWIDGET_H
