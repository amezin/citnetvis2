#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include <QString>
#include <QHash>

class Identifier
{
public:
    Identifier(const QString &s = QString()) : str(s), hash(qHash(s)) { }

    const QString &toString() const { return str; }
    uint getHash() const { return hash; }

    operator bool() const { return !str.isEmpty(); }

    static int compare(const Identifier &a, const Identifier &b)
    {
        if (a.hash < b.hash) return -1;
        if (a.hash > b.hash) return 1;

        return QString::compare(a.str, b.str);
    }

private:
    QString str;
    uint hash;
};

inline uint qHash(const Identifier &s) { return s.getHash(); }

template<typename OStream>
OStream &operator <<(OStream &s, const Identifier &str)
{
    return s << str.toString();
}

#define IDENTIFIER_MAKEOP(op) \
inline bool operator op(const Identifier &a, const Identifier &b) \
{ \
    return Identifier::compare(a, b) op 0; \
}

IDENTIFIER_MAKEOP(==)
IDENTIFIER_MAKEOP(!=)
IDENTIFIER_MAKEOP(<)
IDENTIFIER_MAKEOP(>)
IDENTIFIER_MAKEOP(<=)
IDENTIFIER_MAKEOP(>=)

#undef HASHEDSTRING_MAKEOP

#endif // IDENTIFIER_H
