#include "datasettingswidget.h"

#include <QFormLayout>

DataSettingsWidget::DataSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    auto layout = new QFormLayout(this);
    setLayout(layout);

    endpointUrlEdit = new PersistentField("Endpoint",
                                          "http://acm.rkbexplorer.com/sparql/",
                                          this);
    layout->addRow("&Endpoint URL", endpointUrlEdit);

    titleEdit = new PersistentField("TitlePredicate", "akt:has-title", this);
    layout->addRow("\"&Title\" predicate", titleEdit);

    dateEdit = new PersistentField("DatePredicate", "akt:has-date", this);
    layout->addRow("\"&Date\" predicate", dateEdit);

    dateRegExEdit = new PersistentField("DateRegEx", "[0-9]{4}", this);
    layout->addRow("Date substring", dateRegExEdit);

    referenceEdit = new PersistentField("ReferencePredicate",
                                        "akt:cites-publication-reference",
                                        this);
    layout->addRow("\"&Cites\" predicate", referenceEdit);
}
