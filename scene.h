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

#include "dataset.h"
#include "vnode.h"

class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Scene(QObject *parent = 0);
    
    void setDataset(const Dataset &);

    enum Parameter
    {
        RadiusBase,
        RadiusK,
        Spacing,
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

        NParameters
    };
    qreal parameters[NParameters];

    typedef QLinkedList<VNodeRef> Layer;

    QString selectedNode() const;

public slots:
    void absoluteCoords();
    void build();

private:
    typedef QPair<QString, int> LayerId;

    int computeSubLevel(const Identifier &p, QSet<Identifier> &inStack);

    void addEdge(const Publication &a, const Publication &b);
    void removeOldNodes();
    void fixPublicationInfoAndDate();
    void findEdgesInsideLayers();
    void clearAdjacencyData();
    void arrangeToLayers();
    bool applyForces(Scene::Layer &l);
    void placeLabels();
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

    void sortNodes(Layer &, bool requireSortedNeighbors = true);

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
};

#endif // SCENE_H
