#include "visualisationsettingswidget.h"

#include <QFormLayout>
#include <QDoubleSpinBox>

VisualisationSettingsWidget::VisualisationSettingsWidget(Scene *scene,
                                                         QWidget *parent)
    : QWidget(parent), scene(scene), blockUpdates(false)
{
    layout = new QFormLayout(this);

    for (int i = 0; i < Scene::NParameters; i++) {
        spinBox[i] = 0;
    }

    addSpinBox(Scene::RadiusBase, "&Base radius", 1.0, 100.0);
    addSpinBox(Scene::RadiusK, "&Radius K", 1.0, 100.0);
    addSpinBox(Scene::Spacing, "&Spacing", 1.0, 100.0);
    addSpinBox(Scene::MinLayerWidth, "Min. &layer width", 25.0, 1000.0);
    addSpinBox(Scene::MaxEdgeSlope, "Max. &edge slope", 0.5, 5.0);
    addSpinBox(Scene::EdgeThickness, "Edge &thickness", 1.0, 10.0);
    addSpinBox(Scene::FontSize, "&Font size", 5.0, 32.0);
    addSpinBox(Scene::AnimationDuration, "&Animation duration", 0.1, 5.0);

    addSpinBox(Scene::EdgeSaturation, "Edge color saturation", 0.0, 1.0);
    addSpinBox(Scene::EdgeValue, "Edge color value", 0.0, 1.0);

    addSpinBox(Scene::TextSaturation, "Text color saturation", 0.0, 1.0);
    addSpinBox(Scene::TextValue, "Text color value", 0.0, 1.0);

    addSpinBox(Scene::AdditionalNodeSaturation,
               "Referenced node color saturation", 0.0, 1.0);
    addSpinBox(Scene::AdditionalNodeValue,
               "Referenced node color value", 0.0, 1.0);

    addSpinBox(Scene::LabelPlacementTime, "Label placement timeout", 0.0, 1.0);
}

void VisualisationSettingsWidget::updateSceneParameters()
{
    bool changed = false;

    for (int i = 0; i < Scene::NParameters; i++) {
        if (!spinBox[i]) {
            continue;
        }
        if (!sender() || sender() == spinBox[i]) {
            auto value = static_cast<qreal>(spinBox[i]->value());
            changed = changed || (scene->parameters[i] != value);
            scene->parameters[i] = value;
        }
    }

    if (changed && !blockUpdates) {
        scene->absoluteCoords();
    }
}

void VisualisationSettingsWidget::saveState(QSettings *settings) const
{
    for (int i = 0; i < Scene::NParameters; i++) {
        if (!spinBox[i]) {
            continue;
        }
        settings->setValue(spinBox[i]->objectName(), spinBox[i]->value());
    }
}

void VisualisationSettingsWidget::loadState(const QSettings *settings)
{
    blockUpdates = true;

    for (int i = 0; i < Scene::NParameters; i++) {
        if (!spinBox[i] || !settings->contains(spinBox[i]->objectName())) {
            continue;
        }
        bool ok = false;
        auto v = settings->value(spinBox[i]->objectName()).toDouble(&ok);
        if (ok) {
            spinBox[i]->setValue(v);
        }
    }

    blockUpdates = false;
    updateSceneParameters();
}

void VisualisationSettingsWidget::addSpinBox(Scene::Parameter p,
                                             const QString &title,
                                             double minValue, double maxValue)
{
    Q_ASSERT(!spinBox[p]);
    spinBox[p] = new QDoubleSpinBox(this);
    spinBox[p]->setObjectName(QString("SceneParameter") + p);
    spinBox[p]->setRange(minValue, maxValue);
    spinBox[p]->setValue(scene->parameters[p]);
    layout->addRow(title, spinBox[p]);
    connect(spinBox[p], SIGNAL(valueChanged(double)),
            SLOT(updateSceneParameters()));
}
