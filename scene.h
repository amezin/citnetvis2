#ifndef SCENE_H
#define SCENE_H

#include <QGraphicsScene>
#include <QPair>
#include <QColor>
#include <QSharedPointer>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QLinkedList>

#include "dataset.h"
#include "vnode.h"

class Scene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Scene(QObject *parent = 0);
    
    void setDataset(const Dataset &);

public slots:
    void reLayout();

private:

    qreal layerWidth, spacing, radiusBase, radiusK;

    typedef QPair<QString, int> LayerId;

    void removeCycles(const Identifier &p,
                      QSet<Identifier> &visited,
                      QSet<Identifier> &inStack);

    int computeSubLevel(const Identifier &p);

    void addEdge(const Publication &a, const Publication &b);
    void removeOldNodes();
    void fixPublicationInfoAndDate();
    void findEdgesInsideLayers();
    void clearAdjacencyData();
    void removeCycles();
    void arrangeToLayers();

    struct PublicationInfo
    {
        PublicationInfo();

        QColor color;
        int reverseDeg;
    };

    QHash<Identifier, PublicationInfo> publicationInfo;

    typedef QLinkedList<VNodeRef> Layer;
    void sortNodes(Layer &, bool requireSortedNeighbors = true);

    QMap<LayerId, Layer> layers;

    QHash<Identifier, Publication> publications;
    QHash<Identifier, QSet<Identifier> > inLayerEdges;
    QHash<Identifier, int> subLevels;

    QHash<Identifier, QSharedPointer<QGraphicsEllipseItem> > nodeMarkers;
    QHash<QPair<Identifier, Identifier>, QSharedPointer<QGraphicsLineItem> >
    edgeLines;
};

#endif // SCENE_H
