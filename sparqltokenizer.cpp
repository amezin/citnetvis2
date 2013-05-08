#include "sparqltokenizer.h"

#include <QSet>

SparqlTokenizer::SparqlTokenizer(const QStringRef &s)
    : remaining(s), currentType(Unknown), state(0)
{
    nextChar();
}

static bool decDigit(QChar c)
{
    if (c >= '0' && c <= '9') return true;
    return false;
}

static bool hexValue(const QStringRef &s, QChar &v)
{
    int n = 0;

    for (int i = 0; i < s.size(); i++) {
        n *= 16;
        QChar c = s.at(i);
        if (decDigit(c)) {
            n += c.toLatin1() - L'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            n += c.toLatin1() - L'A' + 10;
        } else {
            return false;
        }
    }

    v = static_cast<QChar>(n);
    return true;
}

static QStringRef takeFirstChars(int len, QStringRef &from)
{
    Q_ASSERT(from.position() >= 0);
    Q_ASSERT(from.size() >= len);
    Q_ASSERT(from.position() + from.size() <= from.string()->length());

    QStringRef to(from.string(), from.position(), len);
    from = QStringRef(from.string(), from.position() + len, from.size() - len);
    return to;
}

bool SparqlTokenizer::nextChar()
{
    static const QString uPrefix("\\u");
    static const QString UPrefix("\\U");

    if (remaining.startsWith(uPrefix) && remaining.size() >= 6) {
        if (hexValue(QStringRef(remaining.string(),
                                remaining.position() + 2, 4), currentChar))
        {
            currentCharRef = takeFirstChars(6, remaining);
            return true;
        }
    } else if (remaining.startsWith(UPrefix) && remaining.size() >= 10) {
        if (hexValue(QStringRef(remaining.string(),
                                remaining.position() + 2, 8), currentChar))
        {
            currentCharRef = takeFirstChars(8, remaining);
            return true;
        }
    }

    if (remaining.isEmpty()) {
        currentCharRef = remaining;
        return false;
    } else {
        currentCharRef = takeFirstChars(1, remaining);
        currentChar = currentCharRef.at(0);
        return true;
    }
}

bool SparqlTokenizer::acceptChar()
{
    currentBuffer.append(currentChar);

    if (current.position() + current.size() != currentCharRef.position()) {
        current = currentCharRef;
        return nextChar();
    }

    current = QStringRef(currentCharRef.string(), current.position(),
                         current.size() + currentCharRef.size());
    return nextChar();
}

static bool enLetter(QChar c)
{
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    return false;
}

static bool pnCharsBase(QChar c)
{
    if (enLetter(c)) return true;
    if (c >= 0x00C0 && c <= 0x00D6) return true;
    if (c >= 0x00D8 && c <= 0x00F6) return true;
    if (c >= 0x00F8 && c <= 0x02FF) return true;
    if (c >= 0x0370 && c <= 0x037D) return true;
    if (c >= 0x037F && c <= 0x1FFF) return true;
    if (c >= 0x200C && c <= 0x200D) return true;
    if (c >= 0x2070 && c <= 0x218F) return true;
    if (c >= 0x2C00 && c <= 0x2FEF) return true;
    if (c >= 0x3001 && c <= 0xD7FF) return true;
    if (c >= 0xF900 && c <= 0xFDCF) return true;
    if (c >= 0xFDF0 && c <= 0xFFFD) return true;
    return false;
}

static bool pnCharsU(QChar c)
{
    return pnCharsBase(c) || (c == '_');
}

static bool pnChars(QChar c)
{
    if (pnCharsU(c)) return true;
    if (c == '-') return true;
    if (decDigit(c)) return true;
    if (c == 0x00B7) return true;
    if (c >= 0x0300 && c <= 0x036F) return true;
    if (c >= 0x203F && c <= 0x2040) return true;
    return false;
}

SparqlTokenizer::Token SparqlTokenizer::innerNext(int startState)
{
    current = QStringRef();
    currentBuffer.clear();

    if (startState) {
        Q_ASSERT(startState == '"' || startState == '\'');

        stringLiteral(startState, true);
        return String;
    }

    if (currentCharRef.isEmpty()) {
        return End;
    }

    if (currentChar.isSpace()) {
        do {
            if (!acceptChar()) break;
        } while (currentChar.isSpace());
        return Whitespace;
    }

    if (currentChar == '#') {
        do {
            if (!acceptChar()) break;
        } while (currentChar != '\r' && currentChar != '\n');

        return Comment;
    }

    if (currentChar == '"' || currentChar == '\'') {
        auto quote = currentChar;
        if (!acceptChar()) return Unknown;

        bool longStr = false;
        if (currentChar == quote) {
            if (!acceptChar() || currentChar != quote) return String;
            longStr = true;
            if (!acceptChar()) return String;
        }
        stringLiteral(quote, longStr);
        return String;
    }

    if (currentChar == '<') {
        QStringRef startCurrentCharRef(currentCharRef);
        QStringRef startRemaining(remaining);

        static const QString iriProhibited("<\"{}|^`\\");
        while (acceptChar()) {
            if (currentChar == '>') {
                acceptChar();
                return IRI;
            }
            if (iriProhibited.contains(currentChar)) break;
            if (currentChar >= 0 && currentChar <= 0x20) break;
        }

        currentChar = '<';
        currentCharRef = startCurrentCharRef;
        current = QStringRef();
        remaining = startRemaining;
    }

    if (currentChar == '@')
    {
        if (!acceptChar() || !enLetter(currentChar)) return Unknown;
        while (acceptChar()) {
            if (!(enLetter(currentChar) || decDigit(currentChar)
                    || currentChar == '-')) break;
        }
        return LangTag;
    }

    if (currentChar == '?' || currentChar == '$') {
        if (!acceptChar() ||
                !(decDigit(currentChar) || pnCharsU(currentChar)))
        {
            return Unknown;
        }

        while (acceptChar()) {
            if (currentChar == '-' || !pnChars(currentChar)) break;
        }
        return Variable;
    }

    if (pnCharsBase(currentChar)) {
        acceptPnChars();
    }

    if (!currentCharRef.isEmpty() && currentChar == ':') {
        if (acceptChar() && (pnCharsU(currentChar) || decDigit(currentChar))) {
            acceptPnChars();
        }
        return Prefixed;
    }

    static QSet<QString> keywords;
    if (keywords.isEmpty()) {
        keywords << "BASE" << "SELECT" << "ORDER" << "BY" << "FROM" << "GRAPH"
                 << "STR" << "ISURI" << "PREFIX" << "CONSTRUCT" << "LIMIT"
                 << "NAMED" << "OPTIONAL" << "LANG" << "ISIRI" << "DESCRIBE"
                 << "OFFSET" << "WHERE" << "UNION" << "LANGMATCHES"
                 << "ISLITERAL" << "ASK" << "DISTINCT" << "FILTER" << "DATATYPE"
                 << "REGEX" << "REDUCED" << "A" << "BOUND" << "TRUE"
                 << "SAMETERM" << "FALSE";
    }
    if (keywords.contains(word().toUpper())) {
        return Keyword;
    }

    acceptChar();
    return Unknown;
}

void SparqlTokenizer::acceptPnChars()
{
    QChar last = 0;
    QStringRef prevCharRef, prevRemaining, prevCurrent;

    while (pnChars(currentChar) || currentChar == '.') {
        if (!acceptChar()) break;
        last = currentChar;
        prevCharRef = currentCharRef;
        prevRemaining = remaining;
        prevCurrent = current;
    }

    if (last == '.') {
        currentCharRef = prevCharRef;
        remaining = prevRemaining;
        current = prevCurrent;
        currentChar = last;
    }
}

void SparqlTokenizer::stringLiteral(QChar quote, bool longStr)
{
    if (longStr) {
        state = quote.unicode();
    } else {
        state = 0;
    }

    bool escaped = false;
    int nquotes = 0;
    do {
        if (!longStr) {
            if (currentChar == '\r' || currentChar == '\n') break;
        }
        if (!escaped && currentChar == quote) {
            nquotes++;
        } else {
            nquotes = 0;
        }
        escaped = (currentChar == '\\' && !escaped);
        if (!acceptChar()) return;
    } while (nquotes < (longStr ? 3 : 1));

    state = 0;
}

SparqlTokenizer::Token SparqlTokenizer::nextSignificant(int startState)
{
    do {
        next(startState);
    } while (currentType == Whitespace || currentType == Comment);
    return currentType;
}

bool SparqlTokenizer::is(const QStringRef &s, SparqlTokenizer::Token t)
{
    SparqlTokenizer tokenizer(s);
    if (tokenizer.next() != t) {
        return false;
    }
    if (tokenizer.next() != End) {
        return false;
    }
    return true;
}
