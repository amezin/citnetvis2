#include "sparqlhighlighter.h"

#include <QTextDocument>

SparqlHighlighter::SparqlHighlighter(QTextDocument *document) :
    QSyntaxHighlighter(document)
{
    colors[SparqlTokenizer::Unknown] = Qt::black;
    colors[SparqlTokenizer::Whitespace] = Qt::black;

    colors[SparqlTokenizer::Comment] = Qt::darkGray;
    colors[SparqlTokenizer::String] = Qt::darkGreen;
    colors[SparqlTokenizer::IRI] = Qt::darkMagenta;
    colors[SparqlTokenizer::Variable] = Qt::darkRed;
    colors[SparqlTokenizer::LangTag] = Qt::darkYellow;
    colors[SparqlTokenizer::Prefixed] = Qt::darkBlue;
    colors[SparqlTokenizer::Keyword] = Qt::darkCyan;
}

void SparqlHighlighter::highlightBlock(const QString &text)
{
    SparqlTokenizer tokens(&text);

    int state = previousBlockState();
    if (state == -1) {
        state = 0;
    }

    while (tokens.next(state) != SparqlTokenizer::End) {
        setFormat(tokens.currentToken().position(),
                  tokens.currentToken().length(),
                  colors[tokens.currentTokenType()]);
        state = 0;
    }
    setCurrentBlockState(tokens.endState());
}
