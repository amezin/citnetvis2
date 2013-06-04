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
#include <QStringList>
#include <QFontMetricsF>

#include "dataset.h"
#include "vnode.h"

class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Scene(QObject *parent = 0);
    
    void setDataset(const Dataset &,
                    bool barycenterHeuristic = false,
                    bool slow = false);

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
        YearLineAlpha,
        YearLineWidth,
        YearFontSize,
        AbsoluteCoordsIter,

        NParameters
    };
    qreal parameters[NParameters];
    bool randomize;

    typedef QLinkedList<VNodeRef> Layer;

    QString selectedNode() const;

    int edgeSegmentCount() const { return edgeLines.size(); }
    int publicationCount() const { return nodeMarkers.size(); }
    int improvementSteps() const { return steps; }
    double totalSeconds() const { return timeElapsed; }

    void placeLabels();
    void absoluteCoords();
    void build();
    void finishAnimations();
    long long intersections();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    typedef QPair<QString, int> LayerId;

    int computeSubLevel(const Identifier &p, QSet<Identifier> &inStack);

    VNodeRef insertNode(Identifier publication, const LayerId &layerId,
                        Identifier edgeStart = Identifier(),
                        Identifier edgeEnd = Identifier());
    void addEdge(const Identifier &a, const Identifier &b);
    void removeOldNodes();
    void fixPublicationInfoAndDate();
    void findEdgesInsideLayers();
    void clearAdjacencyData();
    void arrangeToLayers();
    qreal applyForces(Scene::Layer &l);
    void yearGrid(const QMap<QString, qreal> &yearMinX,
                  const QMap<QString, qreal> &yearMaxX);
    qreal tryPlaceLabel(const QRectF &) const;
    qreal placeLabel(const VNodeRef &n, QRectF rect);

    struct PublicationInfo
    {
        PublicationInfo();

        QString date;
        QColor color;
        int reverseDeg;
        bool showLabel;
    };
    qreal radius(const PublicationInfo &) const;
    qreal radius(const VNodeRef &) const;
    qreal minLayerWidth(const VNodeRef &p, bool prev) const;
    QFontMetricsF fontWithMetrics(QFont &);

    QHash<Identifier, PublicationInfo> publicationInfo;

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

    QVector<QSharedPointer<QGraphicsLineItem> > yearLines;
    QHash<QString, QSharedPointer<QGraphicsSimpleTextItem> >
    yearLabels, oldYearLabels;

    int steps;
    qreal timeElapsed;
};

#endif // SCENE_H
