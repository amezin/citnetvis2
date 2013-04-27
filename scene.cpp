#include "scene.h"

#include <QDebug>
#include <QMutableSetIterator>
#include <QMutableHashIterator>
#include <QMutableMapIterator>
#include <QMutableLinkedListIterator>
#include <QVector>
#include <QtAlgorithms>

Scene::PublicationInfo::PublicationInfo() : reverseDeg(0)
{
    color = QColor::fromHsvF(qrand() / static_cast<qreal>(RAND_MAX), 1, 1);
}

Scene::Scene(QObject *parent) :
    QGraphicsScene(parent), layerWidth(100), spacing(5), radiusBase(5),
    radiusK(5)
{
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
    removeCycles();
    arrangeToLayers();

    clearAdjacencyData();

    foreach (auto i, publications) {
        addEdge(i, i);

        foreach (auto j, i.references) {
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

    reLayout();
}

void Scene::reLayout()
{
    clear();

    qreal x = 0;
    foreach (auto l, layers) {
        qreal y = 0;
        qreal maxR = 0;

        foreach (auto i, l) {
            qreal r = 0;
            if (i->publication) {
                Q_ASSERT(publicationInfo.contains(i->publication));
                r = publicationInfo[i->publication].reverseDeg
                        * radiusK + radiusBase;
            }
            y += r * 2;

            i->x = x;
            i->y = y;

            if (i->publication) {
                QBrush brush(publicationInfo[i->publication].color);
                addEllipse(x - r, y - r, r * 2, r * 2, Qt::NoPen, brush);
            }

            foreach (auto j, i->neighbors) {
                if (j->currentLayer < i->currentLayer) {
                    QColor color = i->edgeColors[j];
                    color.setAlphaF(0.5);

                    QPen pen(color);
                    pen.setWidthF(1.5);
                    addLine(j->x, j->y, i->x, i->y, pen)->setZValue(-1);
                }
            }

            y += r * 2 + spacing;
            maxR = qMax(r, maxR);
        }

        x += qMax(layerWidth, maxR * 2);
    }
}

int Scene::computeSubLevel(const Identifier &p)
{
    if (subLevels.contains(p)) {
        return subLevels[p];
    }

    int maxSubLevel = 0;

    foreach (auto i, inLayerEdges[p]) {
        maxSubLevel = qMax(maxSubLevel, computeSubLevel(i) + 1);
    }

    subLevels[p] = maxSubLevel;
    return maxSubLevel;
}

void Scene::arrangeToLayers()
{
    subLevels.clear();

    QSet<LayerId> usedLayers;
    foreach (auto i, publications) {
        LayerId layer(i.date, computeSubLevel(i.iri()));
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

void Scene::removeCycles(const Identifier &p, QSet<Identifier> &visited,
                         QSet<Identifier> &inStack)
{
    if (visited.contains(p)) {
        return;
    }

    inStack.insert(p);

    QMutableSetIterator<Identifier> i(inLayerEdges[p]);
    while (i.hasNext()) {
        if (inStack.contains(i.next())) {
            qWarning() << "Cycle found. Ignoring edge" << p << "," << i.value();
            i.remove();
        } else {
            removeCycles(i.value(), visited, inStack);
        }
    }

    inStack.remove(p);
    visited.insert(p);
}

void Scene::removeCycles()
{
    QSet<Identifier> visited, inStack;
    for (auto i = inLayerEdges.begin(); i != inLayerEdges.end(); i++) {
        if (i->isEmpty()) {
            removeCycles(i.key(), visited, inStack);
        }
    }
    for (auto i = inLayerEdges.begin(); i != inLayerEdges.end(); i++) {
        removeCycles(i.key(), visited, inStack);
    }
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
            inLayerEdges[i.iri()].insert(j);
        }
    }
}

void Scene::fixPublicationInfoAndDate()
{
    for (auto i = publications.begin(); i != publications.end(); i++) {
        publicationInfo[i.key()].reverseDeg = 0;
    }

    for (auto i = publications.begin(); i != publications.end(); i++) {
        if (i->date.isEmpty()) {
            qWarning() << "No date for publication" << i->iri();

            foreach (auto j, i->references) {
                auto k = publications.find(j);
                if (k != publications.end()) {
                    i->date = qMax(i->date, k->date);
                    publicationInfo[j].reverseDeg++;
                }
            }

            qDebug() << "Changed date for" << i->iri() << "to" << i->date;
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
            prev->edgeColors[*found] = publicationInfo[a.iri()].color;
            (*found)->neighbors.insert(prev);
            (*found)->edgeColors[prev] = publicationInfo[a.iri()].color;
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

static int intersectionNumber(const VNodeRef &a, const VNodeRef &b)
{
    return intersectionNumber(a, b, true) + intersectionNumber(a, b, false);
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
        int intersections = 0;
        auto j = layer.begin();
        auto best = j;
        auto bestIntersections = intersections;

        while (j != layer.end()) {
            intersections -= intersectionNumber(i, *j);
            intersections += intersectionNumber(*j, i);

            if (intersections < bestIntersections) {
                bestIntersections = intersections;
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
