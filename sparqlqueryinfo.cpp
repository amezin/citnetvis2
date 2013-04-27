#include "sparqlqueryinfo.h"

#include <QDebug>

#include "sparqltokenizer.h"

static QUrl cutBrackets(const QString &withBrackets)
{
    QStringRef result(&withBrackets);
    if (result.startsWith('<')) {
        result = QStringRef(result.string(),
                            result.position() + 1, result.length() - 1);
    }
    if (result.endsWith('>')) {
        result = QStringRef(result.string(),
                            result.position(), result.length() - 1);
    }
    return result.toString();
}

SparqlQueryInfo::SparqlQueryInfo(const QStringRef &query)
{
    SparqlTokenizer tokens(query);
    tokens.nextSignificant();

    while (tokens.currentTokenType() != SparqlTokenizer::End) {
        if (tokens.currentTokenType() != SparqlTokenizer::Keyword) {
            tokens.nextSignificant();
            continue;
        }

        QString word(tokens.word());
        if (word.compare("BASE", Qt::CaseInsensitive) == 0) {
            if (tokens.nextSignificant() == SparqlTokenizer::IRI) {
                if (!base.isEmpty()) {
                    qWarning() << "Duplicate BASE in query";
                    continue;
                }

                base = cutBrackets(tokens.word());
                tokens.nextSignificant();
            }
        } else if (word.compare("PREFIX", Qt::CaseInsensitive) == 0) {
            if (tokens.nextSignificant() == SparqlTokenizer::Prefixed) {
                QString currentPrefix(tokens.word());
                if (tokens.nextSignificant() == SparqlTokenizer::IRI) {
                    if (prefix.contains(currentPrefix)) {
                        qWarning() << "Duplicate prefix" << currentPrefix;
                        continue;
                    }

                    prefix.insert(currentPrefix, tokens.word());
                    tokens.nextSignificant();
                }
            }
        } else if (word.compare("FROM", Qt::CaseInsensitive) == 0) {
            bool named = false;
            if (tokens.nextSignificant() == SparqlTokenizer::Keyword
                    && tokens.word().compare("NAMED", Qt::CaseInsensitive) == 0)
            {
                named = true;
                tokens.nextSignificant();
            }
            if (tokens.currentTokenType() == SparqlTokenizer::Prefixed
                    || tokens.currentTokenType() == SparqlTokenizer::IRI)
            {
                (named ? fromNamed : from).append(tokens.word());
                tokens.nextSignificant();
            }
        } else tokens.nextSignificant();
    }

    qDebug() << "Base" << base << "Prefix" << prefix
             << "From" << from << "Named" << fromNamed;
}

QString SparqlQueryInfo::resolve(const QString &prefixed) const
{
    SparqlTokenizer tokens(&prefixed);
    if (tokens.nextSignificant() != SparqlTokenizer::Prefixed) {
        if (!SparqlTokenizer::is(prefixed, SparqlTokenizer::IRI)) {
            qWarning() << "Trying to resolve" << prefixed;
        }
        return prefixed;
    }
    const QString cleanPrefix = tokens.word();
    if (tokens.nextSignificant() != SparqlTokenizer::End) {
        qWarning() << "Trying to resolve" << prefixed;
        return prefixed;
    }

    auto i = prefix.upperBound(cleanPrefix);
    if (i != prefix.begin()) {
        --i;
    }
    if (cleanPrefix.startsWith(i.key())) {
        QString iri(i.value());
        Q_ASSERT(iri[0] == '<');
        Q_ASSERT(iri[iri.length() - 1] == '>');

        iri.insert(iri.length() - 1,
                   cleanPrefix.right(cleanPrefix.length() - i.key().length()));

        if (base.isEmpty()) {
            qDebug() << "Resolved" << prefixed << "to" << iri;
            return iri;
        }

        auto resolved = base.resolved(cutBrackets(iri));
        qDebug() << "Resolved" << prefixed << "to" << resolved;
        return "<" + resolved.toString() + ">";
    } else {
        qWarning() << "Failed to resolve" << prefixed;
        return prefixed;
    }
}

QString SparqlQueryInfo::prologue() const
{
    QString built;

    if (!base.isEmpty()) {
        built.append("BASE ");
        built.append(base.toString());
        built.append("\n");
    }

    for (auto i = prefix.begin(); i != prefix.end(); i++) {
        built.append("PREFIX ");
        built.append(i.key());
        built.append(i.value());
        built.append("\n");
    }

    return built;
}

QString SparqlQueryInfo::buildDataset(const QStringList &from,
                                      const QString &keyword) const
{
    QString built;

    foreach (auto s, from) {
        built.append(keyword);
        built.append(" ");
        built.append(resolve(s));
        built.append("\n");
    }

    return built;
}

QString SparqlQueryInfo::dataset() const
{
    return buildDataset(from, "FROM") + buildDataset(fromNamed, "FROM NAMED");
}
