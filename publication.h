#ifndef PUBLICATION_H
#define PUBLICATION_H

#include <QSet>

#include "identifier.h"

class Publication
{
public:
    explicit Publication(const Identifier &iri, bool fromMainQuery = false)
        : recurse(fromMainQuery), id(iri)
    {
    }

    const Identifier &iri() const { return id; }

    QString title;
    QSet<QString> dates;
    QSet<Identifier> references;
    bool recurse;

    const QString &nonEmptyTitle() const {
        return title.isEmpty() ? id.toString() : title;
    }

private:
    Identifier id;
};

#define PUBLICATION_COMPARE_OP(x) \
inline bool operator x(const Publication &a, const Publication &b) \
{ \
    return a.iri() x b.iri(); \
}

PUBLICATION_COMPARE_OP(==)
PUBLICATION_COMPARE_OP(!=)
PUBLICATION_COMPARE_OP(<=)
PUBLICATION_COMPARE_OP(>=)
PUBLICATION_COMPARE_OP(<)
PUBLICATION_COMPARE_OP(>)

#undef PUBLICATION_COMPARE_OP

#endif // PUBLICATION_H
