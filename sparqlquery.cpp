#include "sparqlquery.h"

#include <QNetworkAccessManager>
#include <QNetworkConfiguration>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#   include <QUrlQuery>
#endif

Q_GLOBAL_STATIC(QNetworkAccessManager, network)

int SparqlQuery::maxParallelQueries()
{
    return 6; //Qt limitation
}

static QByteArray encodeQuery(const QString &query)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QUrl url;
    url.addQueryItem("query", query);
    return url.encodedQuery();
#else
    QUrlQuery q;
    q.addQueryItem("query", query);
    return q.query(QUrl::FullyEncoded).toUtf8();
#endif
}

SparqlQuery::SparqlQuery(const QUrl &endpoint, const QString &query,
                         QObject *parent)
    : QObject(parent), reply(0), errorTag(false), errorSet(false),
      endpointUrl(endpoint), postData(encodeQuery(query))
{
}

void SparqlQuery::exec()
{
    Q_ASSERT(!isExecuted());

    QNetworkRequest request(endpointUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    reply = network()->post(request, postData);
    reply->setParent(this);

    connect(reply, SIGNAL(readyRead()), SLOT(firstDataArrived()));
    connect(reply, SIGNAL(finished()), SLOT(firstDataArrived()));

    connect(reply, SIGNAL(readyRead()), SLOT(dataArrived()));
    connect(reply, SIGNAL(finished()), SLOT(dataArrived()));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            SLOT(networkError()));
}

void SparqlQuery::readData()
{
    switch (xml.readNext()) {
    case QXmlStreamReader::StartElement:
        if (xml.name() == "binding") {
            bindingName = xml.attributes().value("name").toString();
        } else if (xml.name() == "error") {
            errorTag = true;
        }
        return;

    case QXmlStreamReader::EndElement:
        if (xml.name() == "binding") {
            bindingName = QString();
        } else if (xml.name() == "result") {
            data.append(bindings);
            bindings.clear();
        } else if (xml.name() == "error") {
            setError("<error> tag");
        }
        return;

    case QXmlStreamReader::Characters:
        if (errorTag) {
            setError(xml.text().toString());
        } else if (!bindingName.isNull()) {
            bindings[bindingName] = xml.text().toString();
        }
        return;

    case QXmlStreamReader::Invalid:
        if (xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
            setError(xml.errorString());
        }
    case QXmlStreamReader::NoToken:
        if (reply->error() != QNetworkReply::NoError) {
            networkError();
        }
    default:
        return;
    }
}

void SparqlQuery::firstDataArrived()
{
    reply->disconnect(this, SLOT(firstDataArrived()));

    auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (status != 200) {
        setError("HTTP Status " + status.toString());
        return;
    }

    xml.setDevice(reply);
}

void SparqlQuery::dataArrived()
{
    while (!xml.atEnd() && !isFinished()) {
        readData();
    }

    if (hasError()) {
        return;
    }

    if (!data.isEmpty()) {
        emit results(data);
        data.clear();
    }

    if (endOfResults()) {
        emit finished();
    }
}

void SparqlQuery::networkError()
{
    setError(reply->errorString());
}

void SparqlQuery::setError(const QString &err)
{
    Q_ASSERT(!hasError());

    error = err;
    errorSet = true;

    reply->disconnect(this);
    reply->abort();

    emit finished();
}
