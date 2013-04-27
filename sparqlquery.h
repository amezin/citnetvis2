#ifndef SPARQLQUERY_H
#define SPARQLQUERY_H

#include <QObject>
#include <QUrl>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QMap>
#include <QList>

class SparqlQuery : public QObject
{
    Q_OBJECT
public:
    SparqlQuery(const QUrl &endpoint, const QString &query,
                QObject *parent = 0);
    
    const QUrl &endpoint() const { return endpointUrl; }
    bool isExecuted() const { return reply != 0; }

    bool hasError() const { return errorSet; }
    const QString &errorString() const { return error; }

    bool isFinished() const { return hasError() || endOfResults(); }
    bool endOfResults() const
    {
        return xml.tokenType() == QXmlStreamReader::EndDocument;
    }

    static int maxParallelQueries();

    typedef QMap<QString, QString> Bindings;
    typedef QList<Bindings> Results;

public slots:
    void exec();
    void abort() { reply->abort(); }

signals:
    void results(const SparqlQuery::Results &);
    void finished();

private slots:
    void dataArrived();
    void firstDataArrived();
    void networkError();

private:
    void readData();
    void setError(const QString &);

    Results data;
    Bindings bindings;
    QString bindingName;

    QNetworkReply *reply;
    QXmlStreamReader xml;

    bool errorTag;

    QString error;
    bool errorSet;

    QUrl endpointUrl;
    QByteArray postData;
};

#endif // SPARQLQUERY_H
