#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QSettings>
#include <QScrollArea>

#include "logwidget.h"
#include "queryeditor.h"
#include "datasettingswidget.h"
#include "dataset.h"
#include "scene.h"
#include "graphview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QSettings *settings, QWidget *parent = 0);

protected:
    virtual void closeEvent(QCloseEvent *);

private slots:
    void dockLocationChanged(Qt::DockWidgetArea area);
    void dockWidgetDestroyed();
    void dockWidgetTopLevelChanged(bool);

    void executeQuery();

    void showGraph();

private:
    QDockWidget *addDockWidget(QWidget *widget,
                               const QString &name,
                               const QString &title,
                               Qt::DockWidgetArea area
                               = Qt::BottomDockWidgetArea);
    void addToDockBar(Qt::DockWidgetArea area, QDockWidget *dockwidget);
    void removeButton(QDockWidget *);
    QToolBar *addDockBar(Qt::ToolBarArea area);
    QScrollArea *makeScrollable(QWidget *widget);

    GraphView *view;
    QMap<Qt::DockWidgetArea, QToolBar *> dockBars;
    QMap<QDockWidget *, QAction *> dockButtons;

    LogWidget *log;
    QueryEditor *query;
    DataSettingsWidget *settingsWidget;

    QSettings *settings;
    Dataset *dataset;
    QAction *stopAction;
    Scene *scene;
};

#endif // MAINWINDOW_H
