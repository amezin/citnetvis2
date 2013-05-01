#include "dataset.h"

#include "sparqltokenizer.h"

#include <QTimer>
#include <QDebug>

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

void Dataset::addPublications(const SparqlQuery::Results &results)
{
    foreach (auto i, results) {
        if (i.size() != 1) {
            setError("Query must return a list of publications");
            return;
        }

        Identifier id(i.begin().value());
        auto found = currentPublications.find(id);
        if (found == currentPublications.end()) {
            currentPublications.insert(id, Publication(id, true));
            publicationQueue.append(id.toString());
        } else if (!found->recurse) {
            found->recurse = true;
            foreach (auto ref, found->references) {
                if (!currentPublications.contains(ref)) {
                    currentPublications.insert(ref, Publication(ref));
                    publicationQueue.append(ref.toString());
                }
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
                j->date = date;
            } else {
                j->date = date.mid(idx, dateSubstring.matchedLength());
            }
        } else if (!hasTitleResolved.isEmpty() && pred == hasTitleResolved) {
            j->title = i[object];
        } else if (pred == referenceResolved) {
            Identifier ref(i[object]);
            j->references.insert(ref);

            if (j->recurse && !currentPublications.contains(ref)) {
                currentPublications.insert(ref, Publication(ref));
                publicationQueue.append(ref.toString());
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
