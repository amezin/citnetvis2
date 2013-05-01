#ifndef VISUALISATIONSETTINGSWIDGET_H
#define VISUALISATIONSETTINGSWIDGET_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>

#include "scene.h"
#include "persistentwidget.h"

class VisualisationSettingsWidget : public QWidget, public PersistentWidget
{
    Q_OBJECT
public:
    explicit VisualisationSettingsWidget(Scene *scene, QWidget *parent = 0);

    virtual void saveState(QSettings *) const;
    virtual void loadState(const QSettings *);

private slots:
    void updateSceneParameters();

private:
    void addSpinBox(Scene::Parameter, const QString &title,
                    double minValue, double maxValue);

    Scene *scene;
    QDoubleSpinBox *spinBox[Scene::NParameters];
    QFormLayout *layout;
};

#endif // VISUALISATIONSETTINGSWIDGET_H
