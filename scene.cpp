#include "scene.h"

#include <limits>

#include <QDebug>
#include <QMutableSetIterator>
#include <QMutableHashIterator>
#include <QMutableMapIterator>
#include <QMutableLinkedListIterator>
#include <QVector>
#include <QFontMetricsF>
#include <QElapsedTimer>

#include <QtAlgorithms>
#include <qmath.h>

#include "lineanimation.h"
#include "nodeanimation.h"
#include "labelanimation.h"
#include "opacityanimation.h"
#include "disappearanimation.h"

Scene::PublicationInfo::PublicationInfo() : reverseDeg(0)
{
    color = QColor::fromHsvF(qrand() / static_cast<qreal>(RAND_MAX), 1, 1);
}

Scene::Scene(QObject *parent) :
    QGraphicsScene(parent)
{
    parameters[RadiusBase] = 5;
    parameters[RadiusK] = 5;
    parameters[Spacing] = 7;
    parameters[MinLayerWidth] = 75;
    parameters[MaxEdgeSlope] = 3;
    parameters[EdgeThickness] = 2;

    parameters[EdgeSaturation] = 0.5;
    parameters[EdgeValue] = 1;
    parameters[TextSaturation] = 1;
    parameters[TextValue] = 0.5;
    parameters[AdditionalNodeSaturation] = 0.25;
    parameters[AdditionalNodeValue] = 1;

    parameters[FontSize] = 6;
    parameters[AnimationDuration] = 1;

    parameters[LabelPlacementTime] = 0.1;
}

void Scene::setDataset(const Dataset &ds)
{
    if (ds.hasError()) {
        return;
    }

    publications = ds.publications();
    qDebug() << "Publications:" << publications.size();

    fixPublicationInfoAndDate();

    findEdgesInsideLayers();
    arrangeToLayers();

    clearAdjacencyData();

    foreach (auto i, publications) {
        foreach (auto j, i.references) {
            if (i.iri() == j) {
                continue;
            }

            auto k = publications.find(j);
            if (k == publications.end()) {
                continue;
            }
            addEdge(i, *k);
        }
    }

    removeOldNodes();

    auto i = layers.begin();
    while (i != layers.end()) {
        sortNodes(*i++);
    }
    while (i != layers.begin()) {
        sortNodes(*(--i));
    }

    while (i != layers.end()) {
        sortNodes(*i++, false);
    }
    while (i != layers.begin()) {
        sortNodes(*(--i), false);
    }
    while (i != layers.end()) {
        sortNodes(*i++, false);
    }

    int totalNodes = 0;
    int totalEdges = 0;
    foreach (auto l, layers) {
        totalNodes += l.size();
        foreach (auto n, l) {
            Q_ASSERT(n->indexInLayer >= 0);
            totalEdges += n->neighbors.size();
        }
    }
    qDebug() << "Nodes:" << totalNodes << "Edges:" << totalEdges;

    absoluteCoords();
}

qreal Scene::radius(const PublicationInfo &p) const
{
    return qSqrt(p.reverseDeg) * parameters[RadiusK]
            + parameters[RadiusBase];
}

qreal Scene::radius(const VNodeRef &p) const
{
    if (!p->publication) {
        return parameters[EdgeThickness] / 2;
    }
    Q_ASSERT(publicationInfo.contains(p->publication));
    return radius(publicationInfo[p->publication]);
}

qreal Scene::minLayerWidth(const VNodeRef &p, bool prev) const
{
    qreal w = p->size;
    for (auto n : p->neighbors) {
        if ((n->currentLayer < p->currentLayer) == prev) {
            w = qMax(w, qAbs(p->y - n->y) / parameters[MaxEdgeSlope]);
        }
    }
    return w;
}

static void computeForces(Scene::Layer &l, bool before, bool after)
{
    for (auto n : l) {
        int cnt = 0;
        qreal y = 0;
        for (auto r : n->neighbors) {
            if (((r->currentLayer < n->currentLayer) == before) ||
                    ((r->currentLayer > n->currentLayer) == after))
            {
                y += r->y;
                cnt++;
            }
        }
        if (cnt) {
            n->y = y / cnt;
        }
    }
}

bool Scene::applyForces(Scene::Layer &l)
{
    const auto eps = parameters[Spacing] / 2;

    auto i = l.begin();
    if (i == l.end()) {
        return false;
    }

    bool problem = false;
    for (;;) {
        auto bottom = (*i)->y + (*i)->size / 2;
        auto prev = i;
        if (++i == l.end()) break;
        auto top = (*i)->y - (*i)->size / 2;
        if (bottom > top) {
            (*i)->y = (top + bottom + (*i)->size) / 2 + eps;
            (*prev)->y = (top + bottom - (*prev)->size) / 2 - eps;
            problem = true;
        }
    }
    return problem;
}

void Scene::absoluteCoords()
{
    qDebug() << "Called" << __FUNCTION__;

    for (auto l : layers) {
        qreal y = 0;
        for (auto n : l) {
            n->y = y;
            n->size = 2 * radius(n) + parameters[Spacing];
            y += n->size;
        }
    }

    for (auto i : layers) {
        computeForces(i, true, false);
        while (applyForces(i));
    }

    for (auto i = layers.end(); i != layers.begin();) {
        --i;
        computeForces(*i, false, true);
        while (applyForces(*i));
    }

    for (int iter = 0; iter < 9; iter++) {
        for (auto l : layers) {
            computeForces(l, true, true);
        }
        for (auto l : layers) {
            while (applyForces(l));
        }
    }

    qreal x = 0;
    for (auto l = layers.begin(); l != layers.end(); l++) {
        for (auto n : *l) {
            n->x = x;
        }

        auto width = parameters[MinLayerWidth];
        for (auto n : *l) {
            width = qMax(width, minLayerWidth(n, false));
        }
        auto next = l;
        if (++next != layers.end()) {
            for (auto n : *next) {
                width = qMax(width, minLayerWidth(n, true));
            }
        }

        x += width;
    }

    build();
}

static void runAnim(QVariantAnimation *anim)
{
    auto scene = qobject_cast<Scene *>(anim->parent());
    anim->setDuration(static_cast<int>(
                          scene->parameters[Scene::AnimationDuration] * 1000));
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Scene::addNodeMarker(const VNodeRef &n, qreal r, const QColor &color)
{
    if (!n->publication) {
        return;
    }
    QRectF rect(n->x - r, n->y - r, r * 2, r * 2);

    QSharedPointer<QGraphicsEllipseItem> ptr;
    auto found = oldNodeMarkers.find(n->publication);
    if (found != oldNodeMarkers.end()) {
        ptr = *found;
        ptr->setBrush(color);
        if (ptr->rect() != rect) {
            runAnim(new NodeAnimation(ptr.data(), rect, this));
        }
    } else {
        ptr = QSharedPointer<QGraphicsEllipseItem>(
                    addEllipse(rect, Qt::NoPen, color));
    }
    nodeMarkers.insert(n->publication, ptr);
}

void Scene::addEdgeLine(const VNodeRef &start, const VNodeRef &end,
                        const QPen &pen)
{
    QLineF line(start->x, start->y, end->x, end->y);
    auto key = qMakePair(start, end);
    QSharedPointer<QGraphicsLineItem> ptr;
    auto found = oldEdgeLines.find(key);
    if (found != oldEdgeLines.end()) {
        ptr = *found;
        ptr->setPen(pen);
        if (ptr->line() != line) {
            runAnim(new LineAnimation(ptr.data(), line, this));
        }
    } else {
        ptr = QSharedPointer<QGraphicsLineItem>(addLine(line, pen));
        ptr->setZValue(-1);
    }
    edgeLines.insert(key, ptr);
}

void Scene::addLabel(const VNodeRef &n, const QPointF &pos, const QFont &font,
                     const QBrush &brush)
{
    if (!n->publication) {
        return;
    }

    QSharedPointer<QGraphicsSimpleTextItem> ptr;
    auto found = oldLabels.find(n->publication);
    if (found != oldLabels.end()) {
        ptr = *found;
        ptr->setFont(font);
        ptr->setText(n->label);
        if (ptr->pos() != pos) {
            runAnim(new LabelAnimation(ptr.data(), pos, this));
        }
    } else {
        ptr = QSharedPointer<QGraphicsSimpleTextItem>(
                    addSimpleText(n->label, font));
        ptr->setZValue(1);
        ptr->setPos(pos);
    }

    ptr->setBrush(brush);

    labels.insert(n->publication, ptr);
}

template<class K, class V>
void animateItems(QHash<K, QSharedPointer<V> > &old,
                  const QHash<K, QSharedPointer<V> > &n,
                  Scene *s)
{
    for (auto i = old.begin(); i != old.end(); i++) {
        if (!n.contains(i.key())) {
            runAnim(new DisappearAnimation(i.value(), s));
        }
    }
    for (auto i = n.begin(); i != n.end(); i++) {
        if (!old.contains(i.key())) {
            i.value()->setOpacity(0);
            runAnim(new OpacityAnimation(i.value().data(), 1, s));
        }
    }

    old.clear();
}

void Scene::build()
{
    foreach (QObject *p, children()) {
        auto anim = qobject_cast<QAbstractAnimation *>(p);
        if (anim) {
            anim->setCurrentTime(anim->totalDuration());
            anim->stop();
            delete anim;
        }
    }

    labels.swap(oldLabels);
    edgeLines.swap(oldEdgeLines);
    nodeMarkers.swap(oldNodeMarkers);

    for (auto l : layers) {
        for (auto n : l) {
            if (n->publication) {
                QColor color(n->color);
                if (!publications.find(n->publication)->recurse) {
                    color.setHsvF(color.hueF(),
                                  parameters[AdditionalNodeSaturation],
                                  parameters[AdditionalNodeValue]);
                }
                addNodeMarker(n, radius(n), color);
            }
            for (auto r : n->neighbors) {
                if (n->currentLayer > r->currentLayer) {
                    continue;
                }
                QColor edgeColor = n->edgeColors[r];
                edgeColor.setHsvF(edgeColor.hueF(), parameters[EdgeSaturation],
                                  parameters[EdgeValue]);
                QPen edgePen(edgeColor, parameters[EdgeThickness]);
                edgePen.setCapStyle(Qt::RoundCap);
                addEdgeLine(n, r, edgePen);
            }
        }
    }

    placeLabels();

    animateItems(oldLabels, labels, this);
    animateItems(oldEdgeLines, edgeLines, this);
    animateItems(oldNodeMarkers, nodeMarkers, this);
}

qreal Scene::placeLabel(const VNodeRef &n, QRectF rect)
{
    QRectF node = nodeRects[n];

    rect.moveTopLeft(node.bottomRight());
    auto bestRect = rect;
    auto bestResult = tryPlaceLabel(rect);

    rect.moveBottomRight(node.topLeft());
    auto result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    rect.moveBottomLeft(node.topRight());
    result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    rect.moveTopRight(node.bottomLeft());
    result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    rect.moveCenter(node.center());
    rect.moveTop(node.bottom());
    result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    rect.moveCenter(node.center());
    rect.moveBottom(node.top());
    result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    rect.moveCenter(node.center());
    rect.moveRight(node.left());
    result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    rect.moveCenter(node.center());
    rect.moveLeft(node.right());
    result = tryPlaceLabel(rect);
    if (result < bestResult) {
        bestResult = result;
        bestRect = rect;
    }

    labelRects.insert(n, bestRect);
    return bestResult;
}

void Scene::placeLabels()
{
    labelRects.clear();
    nodeRects.clear();
    for (auto l : layers) {
        for (auto n : l) {
            if (n->publication) {
                auto off = radius(n) / qSqrt(2);
                QRectF nodeRect(n->x - off, n->y - off, off * 2, off * 2);
                nodeRects.insert(n, nodeRect);
            }
        }
    }

    QFont font;
    font.setPointSizeF(parameters[FontSize]);
    QFontMetricsF metrics(font);

    static const qreal ticksPerSec = 1000;
    QElapsedTimer timer;
    timer.start();
    do {
        bool change = false;
        for (auto n = nodeRects.begin(); n != nodeRects.end(); n++) {
            auto old = labelRects[n.key()];
            labelRects.remove(n.key());
            placeLabel(n.key(), metrics.boundingRect(n.key()->label));
            if (labelRects[n.key()] != old) {
                change = true;
            }
        }
        if (!change) {
            break;
        }
    } while (timer.elapsed() / ticksPerSec < parameters[LabelPlacementTime]);

    for (auto n = nodeRects.begin(); n != nodeRects.end(); n++) {
        QColor pubColor;
        pubColor.setHsvF(n.key()->color.hueF(), parameters[TextSaturation],
                         parameters[TextValue]);
        addLabel(n.key(), labelRects[n.key()].topLeft(), font, pubColor);
    }
}

qreal Scene::tryPlaceLabel(const QRectF &rect) const
{
    qreal result = 0;

    foreach (auto r, labelRects) {
        QRectF intersection = rect.intersect(r);
        result += intersection.size().width() * intersection.size().height();
    }

    for (auto n = nodeRects.begin(); n != nodeRects.end(); n++) {
        QRectF intersection = rect.intersect(n.value());
        result += intersection.size().width() * intersection.size().height();
    }

    return result;
}

int Scene::computeSubLevel(const Identifier &p, QSet<Identifier> &inStack)
{
    if (subLevels.contains(p)) {
        return subLevels[p];
    }

    inStack.insert(p);

    int maxSubLevel = 0;

    foreach (auto i, inLayerEdges[p]) {
        if (inStack.contains(i)) {
            qWarning() << "Cycle with (" << p << ',' << i << ")";
            continue;
        }
        maxSubLevel = qMax(maxSubLevel, computeSubLevel(i, inStack) + 1);
    }

    subLevels[p] = maxSubLevel;

    inStack.remove(p);

    return maxSubLevel;
}

void Scene::arrangeToLayers()
{
    subLevels.clear();

    QSet<LayerId> usedLayers;
    QSet<Identifier> inStack;
    foreach (auto i, publications) {
        LayerId layer(i.date, computeSubLevel(i.iri(), inStack));
        if (!layers.contains(layer)) {
            layers.insert(layer, Layer());
        }
        usedLayers.insert(layer);
    }
    QMutableMapIterator<LayerId, Layer> i(layers);
    while (i.hasNext()) {
        i.next();
        if (!usedLayers.contains(i.key())) {
            i.remove();
        }
    }

    int maxSubLevel = 0;
    foreach (auto i, subLevels) {
        maxSubLevel = qMax(maxSubLevel, i);
    }
    qDebug() << "Max subLevel:" << maxSubLevel;
}

void Scene::clearAdjacencyData()
{
    for (auto i = layers.begin(); i != layers.end(); i++) {
        for (auto j = i->begin(); j != i->end(); j++) {
            (*j)->neighbors.clear();
            (*j)->edgeColors.clear();
            (*j)->moveable = false;
        }
    }
}

void Scene::findEdgesInsideLayers()
{
    inLayerEdges.clear();
    foreach (auto i, publications) {
        foreach (auto j, i.references) {
            auto k = publications.find(j);
            if (k == publications.end()) {
                continue;
            }
            if (k->date != i.date) {
                continue;
            }
            if (i.iri() != j) {
                inLayerEdges[i.iri()].insert(j);
            }
        }
    }
}

void Scene::fixPublicationInfoAndDate()
{
    for (auto i = publications.begin(); i != publications.end(); i++) {
        publicationInfo[i.key()].reverseDeg = 0;
    }

    QSet<Identifier> noDate;
    for (auto i = publications.begin(); i != publications.end(); i++) {
        bool changeDate = i->date.isEmpty();
        if (changeDate) {
            qWarning() << "No date for publication" << i->iri();
        }
        foreach (auto j, i->references) {
            auto k = publications.find(j);
            if (k != publications.end()) {
                if (changeDate) {
                    i->date = qMax(i->date, k->date);
                }
                publicationInfo[j].reverseDeg++;
            }
        }
        if (changeDate && !i->date.isEmpty()) {
            qWarning() << "Set date for" << i->iri() << "to" << i->date;
        } else {
            noDate.insert(i.key());
        }
    }

    for (auto i = publications.begin(); i != publications.end(); i++) {
        foreach (auto j, i->references) {
            auto k = publications.find(j);
            if (k != publications.end()) {
                if (noDate.contains(k.key()) && !i->date.isEmpty() &&
                        (i->date < k->date || k->date.isEmpty()))
                {
                    k->date = i->date;
                    qWarning() << "Set date for" << i->iri() << "to" << i->date;
                }
            }
        }
    }

    QMutableHashIterator<Identifier, PublicationInfo> i(publicationInfo);
    while (i.hasNext()) {
        i.next();
        if (!publications.contains(i.key())) {
            i.remove();
        }
    }
}

void Scene::removeOldNodes()
{
    int n = 0;

    for (auto i = layers.begin(); i != layers.end(); i++) {
        QMutableLinkedListIterator<VNodeRef> j(*i);
        while (j.hasNext()) {
            if (j.next()->updated) {
                j.value()->updated = false;
            } else {
                j.remove();
                n++;
            }
        }
    }

    qDebug() << "Removed" << n << "nodes";
}

void Scene::addEdge(const Publication &a, const Publication &b)
{
    LayerId aLayer(a.date, subLevels[a.iri()]);
    LayerId bLayer(b.date, subLevels[b.iri()]);

    auto startIter = layers.lowerBound(qMin(aLayer, bLayer));
    auto endIter = layers.upperBound(qMax(aLayer, bLayer));

    VNodeRef prev;
    for (auto i = startIter; i != endIter; i++)
    {
        VNodeRef expected = VNode();
        if (i.key() == aLayer) {
            expected->publication = a.iri();
        } else if (i.key() == bLayer) {
            expected->publication = b.iri();
        } else {
            expected->edgeStart = a.iri();
            expected->edgeEnd = b.iri();
        }

        auto found = i->constBegin();
        while (found != i->constEnd()) {
            if ((*found)->publication == expected->publication &&
                    (*found)->edgeStart == expected->edgeStart &&
                    (*found)->edgeEnd == expected->edgeEnd)
                break;
            found++;
        }
        if (found == i->constEnd()) {
            expected->currentLayer = i.key();
            i->prepend(expected);
            found = i->begin();
        }

        if (prev) {
            prev->neighbors.insert(*found);
            prev->edgeColors[*found] = publicationInfo[b.iri()].color;
            (*found)->neighbors.insert(prev);
            (*found)->edgeColors[prev] = publicationInfo[b.iri()].color;
        }

        if (i.key() == aLayer) {
            (*found)->color = publicationInfo[a.iri()].color;
            (*found)->label = a.nonEmptyTitle();
        } else if (i.key() == bLayer) {
            (*found)->color = publicationInfo[b.iri()].color;
            (*found)->label = b.nonEmptyTitle();
        }

        (*found)->updated = true;
        prev = *found;
    }
}

static QVector<int> sortedNeighbors(const VNodeRef &n, bool side)
{
    QVector<int> result;
    result.reserve(n->neighbors.size());
    foreach (auto i, n->neighbors) {
        if (i->indexInLayer >= 0 && side == (i->currentLayer < n->currentLayer))
        {
            result.append(i->indexInLayer);
        }
    }

    return result;
}

static int intersectionNumber(const VNodeRef &a, const VNodeRef &b, bool side)
{
    int result = 0;

    auto bNeighbors = sortedNeighbors(b, side);
    qSort(bNeighbors.begin(), bNeighbors.end());
    foreach (auto i, sortedNeighbors(a, side)) {
        auto found = qLowerBound(bNeighbors, i);
        result += found - bNeighbors.constBegin();
    }

    return result;
}

void Scene::sortNodes(Layer &layer, bool requireSortedNeighbors)
{
    QMutableLinkedListIterator<VNodeRef> moveable(layer);
    QVector<VNodeRef> toInsert;
    toInsert.reserve(layer.size());
    while (moveable.hasNext()) {
        if (moveable.next()->moveable) {
            toInsert.append(moveable.value());
            moveable.remove();
        }
    }

    qStableSort(toInsert.begin(), toInsert.end(), VNodeRef::DegreeGreater());

    foreach (auto i, toInsert) {
        int left = 0;
        int right = 0;

        auto j = layer.begin();
        auto best = j;
        auto bestLeft = left;
        auto bestRight = right;

        while (j != layer.end()) {
            left -= intersectionNumber(i, *j, false);
            left += intersectionNumber(*j, i, false);

            right -= intersectionNumber(i, *j, true);
            right += intersectionNumber(*j, i, true);

            if ((left + right) < (bestLeft + bestRight) ||
                    ((left + right) == (bestLeft + bestRight) &&
                     qMin(left, right) < qMin(bestLeft, bestRight)))
            {
                bestLeft = left;
                bestRight = right;
                best = j;
                ++best;
            }

            ++j;
        }

        layer.insert(best, i);
    }

    int idx = 0;
    for (auto i = layer.begin(); i != layer.end(); i++) {
        if (requireSortedNeighbors && (*i)->indexInLayer < 0) {
            bool skip = false;
            foreach (auto j, (*i)->neighbors) {
                if (j->indexInLayer < 0) {
                    skip = true;
                    break;
                }
            }

            if (skip) {
                continue;
            }
        }
        (*i)->indexInLayer = idx++;
    }
}
