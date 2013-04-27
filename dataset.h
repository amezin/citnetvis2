#ifndef DATASET_H
#define DATASET_H

#include <QObject>
#include <QSet>
#include <QHash>
#include <QRegExp>

#include "sparqlquery.h"
#include "sparqlqueryinfo.h"
#include "publication.h"

class Dataset : public QObject
{
    Q_OBJECT
public:
    explicit Dataset(const QUrl &endpointUrl, const QString &query,
                     const QString &hasDate, const QString &hasTitle,
                     const QString &citesPublicationReference,
                     const QString &dateRegEx, QObject *parent = 0);

    bool isFinished() const
    {
        return inProgress.isEmpty() && publicationQueue.isEmpty();
    }

    bool hasError() const { return errorSet; }
    const QString &errorString() const { return error; }

    const QHash<Identifier, Publication> &publications() const
    {
        return currentPublications;
    }

public slots:
    void abort();

signals:
    void finished();
    void progress(int done, int total);

private slots:
    void queryFinished();

    void addPublications(const SparqlQuery::Results &);
    void addProperties(const SparqlQuery::Results &);

    void runQueries();

private:
    SparqlQuery *createQuery(const QString &);
    void emitProgress();
    void setError(const QString &);
    void checkPredicate(const QString &);
    void queryPublication(const QString &);

    QSet<SparqlQuery*> inProgress;
    int totalQueries;

    QUrl endpoint;
    QString hasDateResolved, hasTitleResolved, referenceResolved;
    SparqlQueryInfo queryInfo;
    QRegExp dateSubstring;

    bool errorSet;
    QString error;

    QString queryBegin, queryEnd;
    QStringList publicationQueue;

    QHash<Identifier, Publication> currentPublications;
};

#endif // DATASET_H