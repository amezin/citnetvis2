#ifndef VNODE_H
#define VNODE_H

#include <QVector>
#include <QPair>
#include <QExplicitlySharedDataPointer>
#include <QSharedData>
#include <QHash>
#include <QColor>

#include "identifier.h"

struct VNode;
typedef QExplicitlySharedDataPointer<VNode> VNodeRef;

struct VNode : public QSharedData
{
    VNode() : indexInLayer(-1), updated(true), moveable(true) { }

    Identifier publication, edgeStart, edgeEnd;

    QVector<VNodeRef> neighbors[2];
    QHash<VNodeRef, QColor> edgeColors;
    int indexInLayer;

    bool updated;
    bool moveable;

    QPair<QString, int> currentLayer;
    qreal x, y, size, newY;
    QString label;
    QColor color;

    QVector<int> neighborIndices[2];
};

inline uint qHash(const VNodeRef &ref)
{
    return qHash(ref.data());
}

#endif // VNODE_H
