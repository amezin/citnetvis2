#ifndef SPARQLHIGHLIGHTER_H
#define SPARQLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QMap>

#include "sparqltokenizer.h"

class SparqlHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit SparqlHighlighter(QTextDocument *document);
    
protected:
    virtual void highlightBlock(const QString &text);

private:
    QMap<SparqlTokenizer::Token, QColor> colors;
};

#endif // SPARQLHIGHLIGHTER_H
