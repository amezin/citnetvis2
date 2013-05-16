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
    title->setTextFormat(Qt::RichText);
    title->setOpenExternalLinks(true);

    auto table = new QTableView(this);
    layout->addWidget(table, 1, 0);

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels(QStringList() << "Predicate" << "Object");
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
    if (subject.isEmpty()) {
        return;
    }

    QString q;
    q.append("SELECT ?p ?o ");
    q.append(info.dataset());
    q.append(" WHERE { <");
    q.append(subject);
    q.append("> ?p ?o. }");
    query = QSharedPointer<SparqlQuery>(new SparqlQuery(endpoint, q, this));

    model->removeRows(0, model->rowCount());

    connect(query.data(), SIGNAL(results(SparqlQuery::Results)),
            SLOT(dataArrived(SparqlQuery::Results)));
    query->exec();

    static const QString linkFormat("<a href=\"%1\">%1</a>");
    title->setText(linkFormat.arg(subject));
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
