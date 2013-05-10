#ifndef SPARQLQUERYINFO_H
#define SPARQLQUERYINFO_H

#include <QStringList>
#include <QMap>
#include <QString>
#include <QUrl>

class SparqlQueryInfo
{
public:
    explicit SparqlQueryInfo(const QStringRef &);

    QString resolve(const QString &prefixed) const;
    QString shorten(const QUrl &) const;

    const QUrl &baseIri() const { return base; }
    const QMap<QString, QString> &prefixes() const { return prefix; }
    const QStringList &graphs() const { return from; }
    const QStringList &namedGraphs() const { return fromNamed; }

    QString prologue() const;
    QString dataset() const;

private:
    QString buildDataset(const QStringList &from, const QString &keyword) const;

    QUrl base;
    QMap<QString, QString> prefix;
    QMap<QString, QString> revPrefix;
    QStringList from;
    QStringList fromNamed;
};

#endif // SPARQLQUERYINFO_H
