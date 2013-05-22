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
#include <QGraphicsView>

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
    parameters[VertexSpacing] = 10;
    parameters[EdgeSpacing] = 3;
    parameters[MinLayerWidth] = 75;
    parameters[MaxEdgeSlope] = 3;
    parameters[EdgeThickness] = 2;

    parameters[EdgeSaturation] = 0.5;
    parameters[EdgeValue] = 1;
    parameters[TextSaturation] = 1;
    parameters[TextValue] = 0.5;
    parameters[AdditionalNodeSaturation] = 0.25;
    parameters[AdditionalNodeValue] = 1;

    parameters[FontSize] = 7;
    parameters[AnimationDuration] = 1;

    parameters[LabelPlacementTime] = 0.1;
    parameters[AbsoluteCoordsTime] = 0.2;
}

void Scene::setDataset(const Dataset &ds, bool showIsolated)
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

    for (auto &i : publications) {
        for (auto &j : i.references) {
            if (i.iri() == j) {
                continue;
            }

            auto k = publications.find(j);
            if (k == publications.end()) {
                continue;
            }
            addEdge(i, *k);
        }
        if (showIsolated) {
            insertNode(i.iri(), LayerId(i.date, subLevels[i.iri()]));
        }
    }

    removeOldNodes();

    auto i = layers.begin();
    while (i != layers.end()) {
        sortNodes(*i++, true);
    }
    while (i != layers.begin()) {
        sortNodes(*(--i), false);
    }

    while (i != layers.end()) {
        sortNodes(*i++, true, false);
    }
    while (i != layers.begin()) {
        sortNodes(*(--i), false, false);
    }

    int totalNodes = 0;
    int totalEdges = 0;
    for (auto &l : layers) {
        totalNodes += l.size();
        for (auto &n : l) {
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
    for (auto &n : p->neighbors) {
        if ((n->currentLayer < p->currentLayer) == prev) {
            w = qMax(w, qAbs(p->y - n->y) / parameters[MaxEdgeSlope]);
        }
    }
    return w;
}

static void computeForces(Scene::Layer &l, bool before, bool after)
{
    for (auto &n : l) {
        int cnt = 0;
        qreal y = 0;
        for (auto &r : n->neighbors) {
            if (((r->currentLayer < n->currentLayer) == before) ||
                    ((r->currentLayer > n->currentLayer) == after))
            {
                y += r->y;
                cnt++;
            }
        }
        if (cnt) {
            n->newY = y / cnt;
        } else {
            n->newY = n->y;
        }
    }
}

static bool collision(const VNodeRef &a, const VNodeRef &b)
{
    auto mindist = (a->size + b->size) / 2;
    //return qAbs(a->y - b->y) < mindist * 1.1;
    return a->y + mindist > b->newY || a->newY + mindist > b->y;
}

static const qreal msecsPerSec = 1000;

void Scene::applyForces(Scene::Layer &l)
{
    bool forward = true;
    for (;;) {
        for (auto i = forward ? l.begin() : l.end() - 1; i != l.end();) {
            (*i)->y = (*i)->newY;
            if (i != l.begin()) {
                (*i)->y = qMax((*i)->y, (*(i - 1))->y
                               + ((*(i - 1))->size + (*i)->size) / 2);
            }
            if (i + 1 != l.end()) {
                (*i)->y = qMin((*i)->y, (*(i + 1))->y
                               - ((*(i + 1))->size + (*i)->size) / 2);
            }
            if (forward) i++;
            else {
                if (i == l.begin()) break;
                i--;
            }
        }
        forward = !forward;

        bool blockMoved = false;
        auto i = l.begin();
        while (i != l.end()) {
            qreal common = (*i)->newY - (*i)->y;
            auto start = i++;
            int n = 1;
            while (i != l.end() && collision(*(i - 1), *i)) {
                common += (*i)->newY - (*i)->y;
                i++;
                n++;
            }
            common /= n;
            if (qAbs(common) > 0.1) {
                blockMoved = true;
            }
            while (start != i) {
                (*start)->newY = (*start)->y + common;
                start++;
            }
        }

        if (!blockMoved) {
            return;
        }
    }
}

void Scene::absoluteCoords()
{
    qDebug() << "Called" << __FUNCTION__;

    for (auto &l : layers) {
        qreal y = 0;
        for (auto &n : l) {
            n->size = 2 * radius(n)
                    + parameters[n->publication ? VertexSpacing : EdgeSpacing];
            n->y = y + n->size / 2;
            y += n->size;
        }
    }

    for (auto &i : layers) {
        computeForces(i, true, false);
        applyForces(i);
    }

    for (auto i = layers.end(); i != layers.begin();) {
        --i;
        computeForces(*i, false, true);
        applyForces(*i);
    }

    QElapsedTimer timer;
    timer.start();
    do {
        for (auto &l : layers) {
            computeForces(l, true, true);
            applyForces(l);
        }

        for (auto i = layers.end(); i != layers.begin();) {
            --i;
            computeForces(*i, true, true);
            applyForces(*i);
        }
    } while (timer.elapsed() / msecsPerSec < parameters[AbsoluteCoordsTime]);

    auto minY = std::numeric_limits<qreal>::max();
    for (auto &l : layers) {
        for (auto &n : l) {
            minY = qMin(minY, n->y);
        }
    }
    for (auto &l : layers) {
        for (auto &n : l) {
            n->y -= minY;
        }
    }

    qreal x = 0;
    for (auto l = layers.begin(); l != layers.end(); l++) {
        for (auto &n : *l) {
            n->x = x;
        }

        auto width = parameters[MinLayerWidth];
        for (auto &n : *l) {
            width = qMax(width, minLayerWidth(n, false));
        }
        auto next = l;
        if (++next != layers.end()) {
            for (auto &n : *next) {
                width = qMax(width, minLayerWidth(n, true));
            }
        }

        x += width;
    }

    build();
}

static QVariantAnimation *runAnim(QVariantAnimation *anim)
{
    auto scene = qobject_cast<Scene *>(anim->parent());
    anim->setDuration(static_cast<int>(
                          scene->parameters[Scene::AnimationDuration]
                      * msecsPerSec));
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    return anim;
}

void Scene::addNodeMarker(const VNodeRef &n, qreal r, const QColor &color)
{
    if (!n->publication) {
        return;
    }
    QRectF rect(n->x - r, n->y - r, r * 2, r * 2);
    finalBounds = finalBounds.united(rect);

    QSharedPointer<QGraphicsEllipseItem> ptr;
    auto found = oldNodeMarkers.find(n->publication);
    if (found != oldNodeMarkers.end()) {
        ptr = *found;
        ptr->setBrush(color);
        if (ptr->rect() != rect) {
            disableBSP(runAnim(new NodeAnimation(ptr.data(), rect, this)));
        }
    } else {
        ptr = QSharedPointer<QGraphicsEllipseItem>(
                    addEllipse(rect, Qt::NoPen, color));
        ptr->setFlag(QGraphicsItem::ItemIsSelectable);
        ptr->setToolTip(n->publication.toString());
        ptr->setData(0, n->publication.toString());
    }
    nodeMarkers.insert(n->publication, ptr);
}

QString Scene::selectedNode() const
{
    auto items = selectedItems();
    if (items.size() != 1) {
        return QString();
    }
    return items.first()->data(0).toString();
}

void Scene::addEdgeLine(const VNodeRef &start, const VNodeRef &end,
                        const QPen &pen)
{
    QLineF line(start->x, start->y, end->x, end->y);
    finalBounds = finalBounds.united(QRectF(qMin(line.x1(), line.x2()),
                                            qMin(line.y1(), line.y2()),
                                            qAbs(line.dx()), qAbs(line.dy())));
    auto key = qMakePair(start, end);
    QSharedPointer<QGraphicsLineItem> ptr;
    auto found = oldEdgeLines.find(key);
    if (found != oldEdgeLines.end()) {
        ptr = *found;
        ptr->setPen(pen);
        if (ptr->line() != line) {
            disableBSP(runAnim(new LineAnimation(ptr.data(), line, this)));
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
            disableBSP(runAnim(new LabelAnimation(ptr.data(), pos, this)));
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

void Scene::finishAnimations()
{
    foreach (QObject *p, children()) {
        auto anim = qobject_cast<QAbstractAnimation *>(p);
        if (anim) {
            anim->setCurrentTime(anim->totalDuration());
            delete anim;
        }
    }
}

void Scene::disableBSP(QAbstractAnimation *anim)
{
    if (itemIndexMethod() != QGraphicsScene::NoIndex) {
        qDebug() << "Disabling BSP";
        setItemIndexMethod(QGraphicsScene::NoIndex);
    }

    runningAnimations.insert(anim);
    connect(anim, SIGNAL(destroyed()), SLOT(animationFinished()));
}

void Scene::animationFinished()
{
    runningAnimations.remove(reinterpret_cast<QAbstractAnimation*>(sender()));

    if (runningAnimations.isEmpty() &&
            itemIndexMethod() != QGraphicsScene::BspTreeIndex)
    {
        qDebug() << "Enabling BSP";
        setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    }
}

void Scene::build()
{
    finishAnimations();

    labels.swap(oldLabels);
    edgeLines.swap(oldEdgeLines);
    nodeMarkers.swap(oldNodeMarkers);

    finalBounds = QRectF();

    for (auto &l : layers) {
        for (auto &n : l) {
            if (n->publication) {
                QColor color(n->color);
                if (!publications.find(n->publication)->recurse) {
                    color.setHsvF(color.hueF(),
                                  parameters[AdditionalNodeSaturation],
                                  parameters[AdditionalNodeValue]);
                }
                addNodeMarker(n, radius(n), color);
            }
            for (auto &r : n->neighbors) {
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

    setSceneRect(finalBounds);
}

typedef void (*LabelPlacement)(QRectF &, const QRectF &);

static void placeLabelBottomRight(QRectF &rect, const QRectF &node)
{
    rect.moveTopLeft(node.bottomRight());
}

static void placeLabelTopRight(QRectF &rect, const QRectF &node)
{
    rect.moveBottomLeft(node.topRight());
}

static void placeLabelBottomLeft(QRectF &rect, const QRectF &node)
{
    rect.moveTopRight(node.bottomLeft());
}

static void placeLabelTopLeft(QRectF &rect, const QRectF &node)
{
    rect.moveBottomRight(node.topLeft());
}

static const auto sqrtOf2 = qSqrt(2);

static void placeLabelLeft(QRectF &rect, const QRectF &node)
{
    rect.moveCenter(node.center());
    rect.moveRight(node.left() - node.width() * (sqrtOf2 - 1) / 2);
}

static void placeLabelRight(QRectF &rect, const QRectF &node)
{
    rect.moveCenter(node.center());
    rect.moveLeft(node.right() + node.width() * (sqrtOf2 - 1) / 2);
}

static LabelPlacement placements[] = {
    placeLabelTopLeft,
    placeLabelBottomLeft,
    placeLabelLeft,
    placeLabelTopRight,
    placeLabelBottomRight,
    placeLabelRight
};

qreal Scene::placeLabel(const VNodeRef &n, QRectF rect)
{
    QRectF node = nodeRects[n];

    placements[0](rect, node);
    auto bestRect = rect;
    auto bestResult = tryPlaceLabel(rect);

    for (unsigned i = 1; i < sizeof(placements) / sizeof(*placements); i++) {
        placements[i](rect, node);
        auto result = tryPlaceLabel(rect);
        if (result < bestResult) {
            bestResult = result;
            bestRect = rect;
        }
    }

    labelRects.insert(n, bestRect);
    return bestResult;
}

void Scene::placeLabels()
{
    labelRects.clear();
    nodeRects.clear();
    for (auto &l : layers) {
        for (auto &n : l) {
            if (n->publication) {
                auto off = radius(n) / sqrtOf2;
                QRectF nodeRect(n->x - off, n->y - off, off * 2, off * 2);
                nodeRects.insert(n, nodeRect);
            }
        }
    }

    QFont font;
    font.setPointSizeF(parameters[FontSize]);

    QFontInfo fontInfo(font);
    qDebug() << "Font family:" << fontInfo.family();
    font.setFamily(fontInfo.family());

    QPaintDevice *compatible = 0;
    if (!views().isEmpty()) {
        compatible = static_cast<QPaintDevice*>(views().first());
    }
    QFontMetricsF metrics(font, compatible);

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
    } while (timer.elapsed() / msecsPerSec < parameters[LabelPlacementTime]);

    for (auto n = nodeRects.begin(); n != nodeRects.end(); n++) {
        QColor pubColor;
        pubColor.setHsvF(n.key()->color.hueF(), parameters[TextSaturation],
                         parameters[TextValue]);
        finalBounds = finalBounds.united(labelRects[n.key()]);
        addLabel(n.key(), labelRects[n.key()].topLeft(), font, pubColor);
    }
}

qreal Scene::tryPlaceLabel(const QRectF &rect) const
{
    qreal result = 0;

    for (auto &r : labelRects) {
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

    for (auto &i : inLayerEdges[p]) {
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
    for (auto &i : publications) {
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
    for (auto i : subLevels) {
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
    for (auto &i : publications) {
        for (auto &j : i.references) {
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
        for (auto &j : i->references) {
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
        for (auto &j : i->references) {
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

VNodeRef Scene::insertNode(Identifier publication, const LayerId &layerId,
                           Identifier edgeStart, Identifier edgeEnd)
{
    QColor color;
    QString label;
    if (publication) {
        color = publicationInfo[publication].color;
        label = publications.find(publication)->nonEmptyTitle();
    }

    auto &layer = layers[layerId];
    auto found = layer.constBegin();
    while (found != layer.constEnd()) {
        if ((*found)->publication == publication &&
                (*found)->edgeStart == edgeStart &&
                (*found)->edgeEnd == edgeEnd)
            break;
        found++;
    }

    if (found == layer.constEnd()) {
        VNodeRef expectedRef(new VNode());
        if (publication) {
            expectedRef->color = color;
            expectedRef->label = label;
            expectedRef->publication = publication;
        }
        expectedRef->edgeStart = edgeStart;
        expectedRef->edgeEnd = edgeEnd;
        expectedRef->currentLayer = layerId;
        layer.prepend(expectedRef);
        return expectedRef;
    } else {
        (*found)->color = color;
        (*found)->label = label;
        (*found)->updated = true;
        return *found;
    }
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
        VNodeRef found;
        if (i.key() == aLayer) {
            found = insertNode(a.iri(), i.key());
        } else if (i.key() == bLayer) {
            found = insertNode(b.iri(), i.key());
        } else {
            found = insertNode(Identifier(), i.key(), a.iri(), b.iri());
        }

        if (prev) {
            prev->neighbors.push_back(found);
            prev->edgeColors[found] = publicationInfo[b.iri()].color;
            found->neighbors.push_back(prev);
            found->edgeColors[prev] = publicationInfo[b.iri()].color;
        }

        prev = found;
    }
}

inline int intersectionNumber(const QVector<int> &a, const QVector<int> &b)
{
    int result = 0;
    int j = 0;
    for (int i = 0; i < a.size(); i++) {
        while (j < b.size() && b[j] < a[i]) {
            j++;
        }
        result += j;
    }
    return result;
}

static void sortedNeighbors(const VNodeRef &n, bool layerLess)
{
    n->neighborIndices.resize(0);
    n->nNeighbors = 0;
    for (auto &i : n->neighbors) {
        if ((i->currentLayer < n->currentLayer) != layerLess) {
            continue;
        }
        n->nNeighbors++;
        if (i->indexInLayer >= 0) {
            n->neighborIndices.push_back(i->indexInLayer);
        }
    }
    qSort(n->neighborIndices);
}

void Scene::sortNodes(Layer &layer, bool layerLess, bool requireSortedNeighbors)
{
    for (auto &n : layer) {
        sortedNeighbors(n, layerLess);
        n->sorted = false;
    }

    auto i = layer.begin();
    while (i != layer.end() && (*i)->indexInLayer < 0) {
        auto j = i;
        while (j != layer.begin()) {
            if ((*(j - 1))->neighbors.size() <= (*i)->neighbors.size()) {
                break;
            }
            --j;
        }

        layer.insert(j, *i);
        i = layer.erase(i);
    }

    i = layer.begin();
    while (i != layer.end()) {
        if ((*i)->sorted) {
            i++;
            continue;
        }
        if (requireSortedNeighbors) {
            if ((*i)->neighborIndices.size() < (*i)->nNeighbors) {
                i++;
                continue;
            }
        }
        (*i)->sorted = true;
        if (!(*i)->moveable) {
            i++;
            continue;
        }

        int intersections = 0;

        auto j = layer.begin();
        auto best = j;
        auto bestIntersections = intersections;

        while (j != layer.end()) {
            if (j == i || (*i)->indexInLayer < 0) {
                ++j;
                continue;
            }

            intersections -= intersectionNumber((*i)->neighborIndices,
                                                (*j)->neighborIndices);
            intersections += intersectionNumber((*j)->neighborIndices,
                                                (*i)->neighborIndices);

            if (intersections < bestIntersections)
            {
                bestIntersections = intersections;
                best = j;
                ++best;
            }

            ++j;
        }

        while (best != layer.end() && (*best)->indexInLayer < 0) {
            best++;
        }

        layer.insert(best, *i);
        i = layer.erase(i);
    }

    int idx = 0;
    for (auto &n : layer) {
        if (n->sorted) {
            n->indexInLayer = idx++;
        }
    }
}
