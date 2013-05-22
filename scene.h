#ifndef SCENE_H
#define SCENE_H

#include <QGraphicsScene>
#include <QPair>
#include <QColor>
#include <QSharedPointer>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QLinkedList>
#include <QVariantAnimation>
#include <QSet>

#include "dataset.h"
#include "vnode.h"

class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Scene(QObject *parent = 0);
    
    void setDataset(const Dataset &, bool showIsolated);

    enum Parameter
    {
        RadiusBase,
        RadiusK,
        VertexSpacing,
        MinLayerWidth,
        MaxEdgeSlope,
        EdgeThickness,
        EdgeSaturation,
        EdgeValue,
        TextSaturation,
        TextValue,
        AdditionalNodeSaturation,
        AdditionalNodeValue,
        FontSize,
        AnimationDuration,
        LabelPlacementTime,
        EdgeSpacing,
        AbsoluteCoordsTime,

        NParameters
    };
    qreal parameters[NParameters];

    typedef QLinkedList<VNodeRef> Layer;

    QString selectedNode() const;

    int edgeSegmentCount() const { return edgeLines.size(); }
    int publicationCount() const { return nodeMarkers.size(); }

    void placeLabels();
    void absoluteCoords();
    void build();
    void finishAnimations();

private slots:
    void animationFinished();

private:
    void disableBSP(QAbstractAnimation *);

    typedef QPair<QString, int> LayerId;

    int computeSubLevel(const Identifier &p, QSet<Identifier> &inStack);

    VNodeRef insertNode(Identifier publication, const LayerId &layerId,
                        Identifier edgeStart = Identifier(),
                        Identifier edgeEnd = Identifier());
    void addEdge(const Publication &a, const Publication &b);
    void removeOldNodes();
    void fixPublicationInfoAndDate();
    void findEdgesInsideLayers();
    void clearAdjacencyData();
    void arrangeToLayers();
    void applyForces(Scene::Layer &l);
    qreal tryPlaceLabel(const QRectF &) const;
    qreal placeLabel(const VNodeRef &n, QRectF rect);

    struct PublicationInfo
    {
        PublicationInfo();

        QColor color;
        int reverseDeg;
    };
    qreal radius(const PublicationInfo &) const;
    qreal radius(const VNodeRef &) const;
    qreal minLayerWidth(const VNodeRef &p, bool prev) const;

    QHash<Identifier, PublicationInfo> publicationInfo;

    void sortNodes(Layer &, bool layerLess, bool requireSortedNeighbors = true);

    QMap<LayerId, Layer> layers;

    QHash<Identifier, Publication> publications;
    QHash<Identifier, QSet<Identifier> > inLayerEdges;
    QHash<Identifier, int> subLevels;

    QHash<VNodeRef, QRectF> labelRects;
    QHash<VNodeRef, QRectF> nodeRects;

    QHash<Identifier, QSharedPointer<QGraphicsSimpleTextItem> >
    labels, oldLabels;
    QHash<Identifier, QSharedPointer<QGraphicsEllipseItem> >
    nodeMarkers, oldNodeMarkers;
    QHash<QPair<VNodeRef, VNodeRef>, QSharedPointer<QGraphicsLineItem> >
    edgeLines, oldEdgeLines;

    void addNodeMarker(const VNodeRef &, qreal r, const QColor &);
    void addEdgeLine(const VNodeRef &, const VNodeRef &, const QPen &);
    void addLabel(const VNodeRef &, const QPointF &, const QFont &,
                  const QBrush &);

    QRectF finalBounds;

    QSet<QAbstractAnimation*> runningAnimations;
};

#endif // SCENE_H
