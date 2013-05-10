#ifndef NODEINFOWIDGET_H
#define NODEINFOWIDGET_H

#include <QWidget>
#include <QUrl>
#include <QSharedPointer>
#include <QStandardItemModel>
#include <QLabel>

#include "sparqlquery.h"
#include "sparqlqueryinfo.h"

class NodeInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NodeInfoWidget(QWidget *parent = 0);

    void setEndpoint(const QUrl &, const SparqlQueryInfo &);

public slots:
    void setNode(const QString &subject);

private slots:
    void dataArrived(const SparqlQuery::Results &);

private:
    QSharedPointer<SparqlQuery> query;
    QStandardItemModel *model;
    QLabel *title;

    QUrl endpoint;
    SparqlQueryInfo info;
};

#endif // NODEINFOWIDGET_H
