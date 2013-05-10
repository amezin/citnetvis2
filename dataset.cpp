#include "dataset.h"

#include "sparqltokenizer.h"

#include <QTimer>
#include <QDebug>

struct CacheInfo
{
    QString queryBegin, queryEnd, dateRegEx;
    QUrl endpointUrl;
    Identifier id;

    CacheInfo(const Dataset &ds, const Identifier &id)
        : queryBegin(ds.queryBegin), queryEnd(ds.queryEnd),
          dateRegEx(ds.dateSubstring.pattern()), endpointUrl(ds.endpoint),
          id(id)
    {
    }
};

bool operator ==(const CacheInfo &a, const CacheInfo &b)
{
    return (a.queryBegin == b.queryBegin) && (a.queryEnd == b.queryEnd) &&
            (a.dateRegEx == b.dateRegEx) && (a.endpointUrl == b.endpointUrl) &&
            (a.id == b.id);
}

uint qHash(const CacheInfo &info)
{
    return qHash(info.queryBegin) + qHash(info.queryEnd) + qHash(info.dateRegEx)
            + qHash(info.endpointUrl) + qHash(info.id);
}

typedef QHash<CacheInfo, Publication> PublicationCache;
Q_GLOBAL_STATIC(PublicationCache, cache)

Dataset::Dataset(const QUrl &endpointUrl, const QString &query,
                 const QString &hasDate, const QString &hasTitle,
                 const QString &citesPublicationReference,
                 const QString &dateRegEx, QObject *parent)
    : QObject(parent), endpoint(endpointUrl),
      queryInfo(&query), dateSubstring(dateRegEx), errorSet(false)
{
    if (!endpointUrl.isValid()) setError("Invalid endpoint URL");

    if (!hasDate.isEmpty()) checkPredicate(hasDate);
    if (!hasTitle.isEmpty()) checkPredicate(hasTitle);
    checkPredicate(citesPublicationReference);

    if (hasError()) {
        QTimer::singleShot(0, this, SIGNAL(finished()));
        return;
    }

    if (!hasDate.isEmpty()) hasDateResolved = queryInfo.resolve(hasDate);
    if (!hasTitle.isEmpty()) hasTitleResolved = queryInfo.resolve(hasTitle);
    referenceResolved = queryInfo.resolve(citesPublicationReference);

    auto mainQuery = createQuery(query);
    connect(mainQuery, SIGNAL(results(SparqlQuery::Results)),
            SLOT(addPublications(SparqlQuery::Results)));
    mainQuery->exec();

    queryBegin.append("SELECT ?s ?p ?o\n");
    queryBegin.append(queryInfo.dataset());
    queryBegin.append("\nWHERE{?s ?p ?o. FILTER(");

    queryEnd.append(") FILTER(");
    if (!hasDateResolved.isEmpty()) {
        queryEnd.append("?p=");
        queryEnd.append(hasDateResolved);
        queryEnd.append("||");
    }
    if (!hasTitleResolved.isEmpty()) {
        queryEnd.append("?p=");
        queryEnd.append(hasTitleResolved);
        queryEnd.append("||");
    }
    queryEnd.append("?p=");
    queryEnd.append(referenceResolved);
    queryEnd.append(")}");
}

void Dataset::checkPredicate(const QString &pred)
{
    if (!SparqlTokenizer::is(pred, SparqlTokenizer::IRI) &&
            !SparqlTokenizer::is(pred, SparqlTokenizer::Prefixed))
    {
        setError(pred + " isn't an IRI or prefixed name");
    }
}

void Dataset::abort()
{
    publicationQueue.clear();
    foreach (auto i, inProgress) {
        i->abort();
    }
}

void Dataset::clear()
{
    abort();
    currentPublications.clear();
}

void Dataset::queryFinished()
{
    bool prevFinished = isFinished();

    auto query = qobject_cast<SparqlQuery*>(sender());
    inProgress.remove(query);

    if (query->hasError()) {
        setError(query->errorString());
    }
    query->deleteLater();

    if (inProgress.size() < SparqlQuery::maxParallelQueries()) {
        runQueries();
    }

    if (!prevFinished && isFinished()) {
        qDebug() << "Nothing more to load";

        if (!hasError()) {
            cache()->clear();
            for (auto p : currentPublications) {
                cache()->insert(CacheInfo(*this, p.iri()), p);
            }
        }

        emit finished();
    }
}

SparqlQuery *Dataset::createQuery(const QString &text)
{
    auto q = new SparqlQuery(endpoint, text, this);
    connect(q, SIGNAL(finished()), SLOT(queryFinished()));
    inProgress.insert(q);

    emitProgress();
    return q;
}

void Dataset::emitProgress()
{
    emit progress(dataReceivedFor.size(), currentPublications.size());
}

void Dataset::setError(const QString &err)
{
    if (errorSet) {
        return;
    }

    errorSet = true;
    error = err;

    qCritical() << err.toLocal8Bit().constData();

    abort();
}

QHash<Identifier, Publication>::Iterator
Dataset::queryPublication(const Identifier &id, bool recurse)
{
    auto found = currentPublications.find(id);
    if (found == currentPublications.end()) {
        auto cached = cache()->find(CacheInfo(*this, id));
        if (cached == cache()->end()) {
            qDebug() << "Cache miss" << id;
            found = currentPublications.insert(id, Publication(id, recurse));
            publicationQueue.append(id.toString());
        } else {
            qDebug() << "Cache hit" << id;
            found = currentPublications.insert(id, *cached);
            found->recurse = false;
            dataReceivedFor.insert(id);
        }
    }
    return found;
}

void Dataset::addPublications(const SparqlQuery::Results &results)
{
    foreach (auto i, results) {
        if (i.size() != 1) {
            setError("Query must return a list of publications");
            return;
        }

        auto found = queryPublication(Identifier(i.begin().value()), true);
        if (!found->recurse) {
            found->recurse = true;
            foreach (auto ref, found->references) {
                queryPublication(ref);
            }
        }
    }

    qDebug() << publicationQueue.size() << "new publications";

    if (inProgress.size() < SparqlQuery::maxParallelQueries()) {
        runQueries();
    }
    emitProgress();
}

void Dataset::runQueries()
{
    if (publicationQueue.isEmpty()) {
        return;
    }

    QString query(queryBegin);
    foreach (auto i, publicationQueue) {
        query.append("?s=<");
        query.append(i);
        query.append(">||");
    }
    if (query.endsWith("||")) {
        query.remove(query.length() - 2, 2);
    }
    query.append(queryEnd);

    publicationQueue.clear();

    auto q = createQuery(query);
    connect(q, SIGNAL(results(SparqlQuery::Results)),
            SLOT(addProperties(SparqlQuery::Results)));
    q->exec();
}

void Dataset::addProperties(const SparqlQuery::Results &results)
{
    static const QString subject("s"), predicate("p"), object("o");

    foreach (auto i, results) {
        if (i.size() != 3 || !i.contains(subject) || !i.contains(predicate) ||
                !i.contains(object))
        {
            setError("Unexpected data. "
                     "Wrong generated query or endpoint problems");
            return;
        }

        Identifier id(i[subject]);
        dataReceivedFor.insert(id);
        auto j = currentPublications.find(id);
        if (j == currentPublications.end()) {
            setError("Unexpected subject " + id.toString() +
                     ". Wrong generated query or endpoint problems");
            return;
        }

        QString pred("<");
        pred.append(i[predicate]);
        pred.append(">");

        if (!hasDateResolved.isEmpty() && pred == hasDateResolved) {
            auto date = i[object];
            int idx = date.lastIndexOf(dateSubstring);

            if (idx == -1) {
                qWarning() << "Can't find date substring in" << date;
            } else {
                j->date = date.mid(idx, dateSubstring.matchedLength());
            }
        } else if (!hasTitleResolved.isEmpty() && pred == hasTitleResolved) {
            j->title = i[object];
        } else if (pred == referenceResolved) {
            Identifier ref(i[object]);
            j->references.insert(ref);

            if (j->recurse) {
                queryPublication(ref);
            }
        } else {
            setError("Unexpected predicate " + pred +
                     ". Wrong generated query or endpoint problems");
            return;
        }
    }

    if (inProgress.size() < SparqlQuery::maxParallelQueries()) {
        runQueries();
    }
    emitProgress();
}
