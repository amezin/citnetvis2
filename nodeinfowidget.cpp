#include "nodeinfowidget.h"

#include <QGridLayout>
#include <QStandardItem>
#include <QTableWidget>
#include <QScopedPointer>
#include <QHeaderView>

NodeInfoWidget::NodeInfoWidget(QWidget *parent) :
    QWidget(parent), info(QStringRef())
{
    auto layout = new QGridLayout(this);

    title = new QLabel("Select a node", this);
    layout->addWidget(title, 0, 0);

    auto table = new QTableView(this);
    layout->addWidget(table, 1, 0);

    model = new QStandardItemModel(this);
    table->setModel(model);
    table->verticalHeader()->setVisible(false);
}

void NodeInfoWidget::setEndpoint(const QUrl &url, const SparqlQueryInfo &info)
{
    endpoint = url;
    this->info = info;
}

void NodeInfoWidget::setNode(const QString &subject)
{
    QString q;
    q.append("SELECT ?p ?o ");
    q.append(info.dataset());
    q.append(" WHERE { <");
    q.append(subject);
    q.append("> ?p ?o. }");
    query = QSharedPointer<SparqlQuery>(new SparqlQuery(endpoint, q, this));

    model->clear();
    model->setHorizontalHeaderLabels(QStringList() << "Predicate" << "Object");

    connect(query.data(), SIGNAL(results(SparqlQuery::Results)),
            SLOT(dataArrived(SparqlQuery::Results)));
    query->exec();

    title->setText(subject);
}

void NodeInfoWidget::dataArrived(const SparqlQuery::Results &results)
{
    static const QString pVar("p");
    static const QString oVar("o");
    for (auto i : results) {
        QScopedPointer<QStandardItem> p(
                    new QStandardItem(info.shorten(i[pVar])));
        QScopedPointer<QStandardItem> o(
                    new QStandardItem(info.shorten(i[oVar])));
        model->appendRow(QList<QStandardItem*>() << p.data() << o.data());
        p.take();
        o.take();
    }
}
