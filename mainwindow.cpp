#include "mainwindow.h"

#include <QToolBar>
#include <QDockWidget>
#include <QAction>
#include <QStatusBar>
#include <QPushButton>
#include <QMenu>
#include <QDebug>
#include <QSvgGenerator>

#include "dockbutton.h"
#include "persistentwidget.h"
#include "dataset.h"
#include "visualisationsettingswidget.h"

static void generateViewMenu(const QObject *widget, QMenu *menu)
{
    auto bar = qobject_cast<const QToolBar *>(widget);
    if (bar) {
        menu->addAction(bar->toggleViewAction());
    }
    auto dock = qobject_cast<const QDockWidget *>(widget);
    if (dock) {
        menu->addAction(dock->toggleViewAction());
    }

    foreach (auto i, widget->children()) {
        generateViewMenu(i, menu);
    }
}

static void loadPersistentWidgets(QObject *root, const QSettings *settings)
{
    auto asPersistentWidget = dynamic_cast<PersistentWidget*>(root);
    if (asPersistentWidget) {
        asPersistentWidget->loadState(settings);
    }

    foreach (auto i, root->children()) {
        loadPersistentWidgets(i, settings);
    }
}

static void savePersistentWidgets(const QObject *root, QSettings *settings)
{
    auto asPersistentWidget = dynamic_cast<const PersistentWidget*>(root);
    if (asPersistentWidget) {
        asPersistentWidget->saveState(settings);
    }

    foreach (auto i, root->children()) {
        savePersistentWidgets(i, settings);
    }
}

MainWindow::MainWindow(QSettings *settings, QWidget *parent)
    : QMainWindow(parent), settings(settings), dataset(0)
{
    setWindowTitle(settings->applicationName());

    scene = new Scene(this);
    connect(scene, SIGNAL(selectionChanged()), SLOT(selectedNodeChanged()));
    view = new GraphView(scene, this);
    setCentralWidget(view);

    auto toolBar = new QToolBar("Main tool bar", this);
    toolBar->setObjectName("MainToolBar");
    addToolBar(toolBar);

    auto queryAction = toolBar->addAction("Run query");
    queryAction->setIcon(QIcon::fromTheme("view-refresh"));
    queryAction->setShortcut(QKeySequence::Refresh);
    connect(queryAction, SIGNAL(triggered()), SLOT(executeQuery()));

    stopAction = toolBar->addAction("Stop");
    stopAction->setIcon(QIcon::fromTheme("process-stop"));
    stopAction->setShortcut(QKeySequence::Quit);
    stopAction->setEnabled(false);

    auto clearAction = toolBar->addAction("Clear");
    clearAction->setIcon(QIcon::fromTheme("document-new"));
    clearAction->setShortcut(QKeySequence::New);
    connect(clearAction, SIGNAL(triggered()), SLOT(clear()));

    auto exportAction = toolBar->addAction("Export");
    exportAction->setIcon(QIcon::fromTheme("document-save-as"));
    exportAction->setShortcut(QKeySequence::Save);
    exportAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(exportAction, SIGNAL(triggered()), SLOT(exportImage()));

    view->addActions(toolBar->actions());
    view->setContextMenuPolicy(Qt::ActionsContextMenu);

    query = new QueryEditor(this);
    addDockWidget(query, "QueryEditor", "Query editor");

    settingsWidget = new DataSettingsWidget(this);
    addDockWidget(makeScrollable(settingsWidget), "DataSettings", "Settings");

    addDockWidget(makeScrollable(new VisualisationSettingsWidget(scene, this)),
                  "VisSettings", "Size and color");

    log = new LogWidget(this);
    auto logDock = addDockWidget(log, "Log", "Errors and warnings");
    logDock->hide();
    connect(log, SIGNAL(message()), logDock, SLOT(show()));

    nodeWidget = new NodeInfoWidget(this);
    nodeDock = addDockWidget(nodeWidget, "NodeInfo", "Node Info",
                             Qt::LeftDockWidgetArea);
    nodeDock->setVisible(false);

    dockBars[Qt::TopDockWidgetArea] = addDockBar(Qt::TopToolBarArea);
    dockBars[Qt::BottomDockWidgetArea] = addDockBar(Qt::BottomToolBarArea);
    dockBars[Qt::LeftDockWidgetArea] = addDockBar(Qt::LeftToolBarArea);
    dockBars[Qt::RightDockWidgetArea] = addDockBar(Qt::RightToolBarArea);

    restoreGeometry(settings->value("geometry").toByteArray());
    restoreState(settings->value("state").toByteArray());

    statusLabel = new QLabel(this);
    dockBars[Qt::BottomDockWidgetArea]->addWidget(statusLabel);
    statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto viewButton = new QPushButton("&View", this);
    dockBars[Qt::BottomDockWidgetArea]->addWidget(viewButton);
    auto viewMenu = new QMenu(viewButton->text(), this);
    viewButton->setMenu(viewMenu);

    foreach (auto i, children()) {
        auto asDockWidget = qobject_cast<QDockWidget*>(i);
        if (asDockWidget && !asDockWidget->isFloating()) {
            addToDockBar(dockWidgetArea(asDockWidget), asDockWidget);
        }
    }

    generateViewMenu(this, viewMenu);

    loadPersistentWidgets(this, settings);

    exportDialog = new QFileDialog(this);
    exportDialog->setAcceptMode(QFileDialog::AcceptSave);
    exportDialog->setDefaultSuffix("svg");
    exportDialog->setNameFilters(QStringList()
                                 << "SVG images (*.svg)" << "All files (*)");
}

QScrollArea *MainWindow::makeScrollable(QWidget *widget)
{
    auto area = new QScrollArea(this);
    area->setWidget(widget);
    area->setWidgetResizable(true);
    area->setFrameStyle(QFrame::NoFrame);
    //widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    return area;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    settings->setValue("geometry", saveGeometry());
    settings->setValue("state", saveState());

    savePersistentWidgets(this, settings);

    QMainWindow::closeEvent(e);
}

QToolBar *MainWindow::addDockBar(Qt::ToolBarArea area)
{
    auto bar = new QToolBar(this);
    switch (area) {
    case Qt::TopToolBarArea: bar->setObjectName("TopDockBar"); break;
    case Qt::BottomToolBarArea: bar->setObjectName("BottomDockBar"); break;
    case Qt::LeftToolBarArea: bar->setObjectName("LeftDockBar"); break;
    case Qt::RightToolBarArea: bar->setObjectName("RightDockBar"); break;
    default: Q_ASSERT_X(false, __FUNCTION__, "Invalid area");
    }

    bar->toggleViewAction()->setVisible(false);
    if (area == Qt::LeftToolBarArea || area == Qt::RightToolBarArea) {
        bar->setOrientation(Qt::Vertical);
    }
    bar->setAllowedAreas(area);
    bar->setFloatable(false);
    addToolBar(area, bar);
    return bar;
}

void MainWindow::dockLocationChanged(Qt::DockWidgetArea area)
{
    addToDockBar(area, qobject_cast<QDockWidget *>(sender()));
}

void MainWindow::dockWidgetTopLevelChanged(bool topLevel)
{
    if (topLevel) {
        addToDockBar(Qt::NoDockWidgetArea,
                     qobject_cast<QDockWidget*>(sender()));
    }
}

void MainWindow::removeButton(QDockWidget *dockwidget)
{
    if (!dockButtons.contains(dockwidget)) {
        return;
    }

    foreach (auto i, dockBars) {
        i->removeAction(dockButtons[dockwidget]);
    }
    auto action = dockButtons[dockwidget];
    dockButtons.remove(dockwidget);
    delete action;
}

void MainWindow::dockWidgetDestroyed()
{
    removeButton(reinterpret_cast<QDockWidget *>(sender()));
}

QDockWidget *MainWindow::addDockWidget(QWidget *widget,
                                       const QString &name,
                                       const QString &title,
                                       Qt::DockWidgetArea area)
{
    QDockWidget *dockWidget = new QDockWidget(title, this);
    dockWidget->setWidget(widget);
    dockWidget->setObjectName(name);
    QMainWindow::addDockWidget(area, dockWidget);

    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
            SLOT(dockLocationChanged(Qt::DockWidgetArea)));
    connect(dockWidget, SIGNAL(destroyed()), SLOT(dockWidgetDestroyed()));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
            SLOT(dockWidgetTopLevelChanged(bool)));

    return dockWidget;
}

void MainWindow::addToDockBar(Qt::DockWidgetArea area, QDockWidget *dockwidget)
{
    removeButton(dockwidget);

    if (!dockBars.contains(area)) {
        dockwidget->toggleViewAction()->setVisible(true);
        return;
    }

    dockwidget->toggleViewAction()->setVisible(false);
    auto button = new DockButton(dockwidget->toggleViewAction(),
                                 dockBars[area]);
    dockButtons[dockwidget] = dockBars[area]->addWidget(button);
}

void MainWindow::executeQuery()
{
    log->clear();

    stopAction->setEnabled(false);
    delete dataset;
    dataset = new Dataset(settingsWidget->endpointUrl(),
                          query->text(),
                          settingsWidget->datePredicate(),
                          settingsWidget->titlePredicate(),
                          settingsWidget->referencePredicate(),
                          settingsWidget->dateRegEx(),
                          settingsWidget->loadReferences(), this);

    connect(dataset, SIGNAL(progress(int,int)), view->progressOverlay(),
            SLOT(setProgress(int,int)));
    connect(dataset, SIGNAL(finished()), view->progressOverlay(), SLOT(done()));

    dataset->connect(stopAction, SIGNAL(triggered()), SLOT(abort()));
    connect(dataset, SIGNAL(finished()), SLOT(showGraph()));
    stopAction->setEnabled(true);

    view->progressOverlay()->setProgress(0, 0);
}

void MainWindow::showGraph()
{
    stopAction->setDisabled(true);
    scene->setDataset(*dataset, settingsWidget->showIsolated(),
                      settingsWidget->useBarycenterHeuristic(),
                      settingsWidget->useSlowAlgorithm());

    static const QString infoText("Publications: %1 Edge segments: %2 "
                                  "Intersections: %3");
    statusLabel->setText(infoText.arg(
                             QString::number(scene->publicationCount()),
                             QString::number(scene->edgeSegmentCount()),
                             QString::number(scene->intersections())));

    nodeWidget->setEndpoint(settingsWidget->endpointUrl(),
                            dataset->queryParameters());
}

void MainWindow::selectedNodeChanged()
{
    nodeDock->setVisible(!scene->selectedNode().isEmpty());
    nodeWidget->setNode(scene->selectedNode());
}

void MainWindow::clear()
{
    dataset->clear();
    scene->setDataset(*dataset, false);
}

void MainWindow::exportImage()
{
    if (!exportDialog->exec() || exportDialog->selectedFiles().size() != 1) {
        return;
    }

    QSvgGenerator svg;
    svg.setFileName(exportDialog->selectedFiles().first());
    svg.setResolution(logicalDpiY());
    scene->finishAnimations();
    scene->clearSelection();
    auto prevRect = scene->sceneRect();
    scene->setSceneRect(scene->itemsBoundingRect());
    svg.setSize(scene->sceneRect().size().toSize());

    QPainter painter(&svg);
    scene->render(&painter);

    scene->setSceneRect(prevRect);
}
