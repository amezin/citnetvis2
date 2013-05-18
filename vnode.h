#ifndef VNODE_H
#define VNODE_H

#include <QVector>
#include <QPair>
#include <QExplicitlySharedDataPointer>
#include <QSharedData>
#include <QHash>
#include <QColor>

#include "identifier.h"

struct VNodeRef;

struct VNode : public QSharedData
{
    VNode() : indexInLayer(-1), updated(true), moveable(true) { }

    Identifier publication, edgeStart, edgeEnd;

    QVector<VNodeRef> neighbors;
    QHash<VNodeRef, QColor> edgeColors;
    int indexInLayer;

    bool updated;
    bool moveable;

    QPair<QString, int> currentLayer;
    qreal x, y, size, newY;
    QString label;
    QColor color;
};

struct VNodeRef : public QExplicitlySharedDataPointer<VNode>
{
    VNodeRef(VNode *ptr = 0) : QExplicitlySharedDataPointer<VNode>(ptr) { }
    VNodeRef &operator =(VNode *ptr)
    {
        *static_cast<QExplicitlySharedDataPointer<VNode> *>(this) = ptr;
        return *this;
    }

    VNodeRef(const VNode &n) : QExplicitlySharedDataPointer<VNode>(new VNode(n))
    {
    }
    VNodeRef &operator =(const VNode &n)
    {
        VNodeRef ref(new VNode(n));
        return *this = ref;
    }

    struct IndexInLayerLess
    {
        bool operator ()(const VNodeRef &a, const VNodeRef &b)
        {
            return a->indexInLayer < b->indexInLayer;
        }
    };

    struct DegreeGreater
    {
        bool operator ()(const VNodeRef &a, const VNodeRef &b)
        {
            return a->neighbors.size() > b->neighbors.size();
        }
    };
};

inline uint qHash(const VNodeRef &ref)
{
    return qHash(ref.data());
}

#endif // VNODE_H
