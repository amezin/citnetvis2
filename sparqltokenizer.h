#ifndef SPARQLTOKENIZER_H
#define SPARQLTOKENIZER_H

#include <QStringRef>

class SparqlTokenizer
{
public:

    explicit SparqlTokenizer(const QStringRef &s);

    enum Token
    {
        Unknown,
        End,
        Whitespace,
        Comment,
        String,
        IRI,
        Variable,
        LangTag,
        Prefixed,
        Keyword
    };

    Token next(int state = 0) { return currentType = innerNext(state); }
    Token nextSignificant(int state = 0);
    const QStringRef &currentToken() const { return current; }
    Token currentTokenType() const { return currentType; }
    int endState() const { return state; }
    QString word() const { return currentBuffer; }

    static bool is(const QStringRef &, Token);
    static bool is(const QString &s, Token t) { return is(&s, t); }

private:
    Token innerNext(int);
    void acceptPnChars();
    void stringLiteral(QChar, bool longStr);

    bool nextChar();
    bool acceptChar();

    QStringRef remaining;
    Token currentType;
    QStringRef current;
    QStringRef currentCharRef;
    QChar currentChar;
    QString currentBuffer;

    int state;
};

#endif // SPARQLTOKENIZER_H
