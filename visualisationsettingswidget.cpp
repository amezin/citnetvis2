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
    changesSize.insert(Scene::RadiusBase);
    addSpinBox(Scene::RadiusK, "&Radius K", 1.0, 100.0);
    changesSize.insert(Scene::RadiusK);
    addSpinBox(Scene::VertexSpacing, "&Vertex Spacing", 1.0, 100.0);
    changesSize.insert(Scene::VertexSpacing);
    addSpinBox(Scene::EdgeSpacing, "Edge &spacing", 1.0, 100.0);
    changesSize.insert(Scene::EdgeSpacing);
    addSpinBox(Scene::MinLayerWidth, "Min. &layer width", 25.0, 1000.0);
    changesColor.insert(Scene::MinLayerWidth);
    changesLabel.insert(Scene::MinLayerWidth);
    addSpinBox(Scene::MaxEdgeSlope, "Max. &edge slope", 0.5, 5.0);
    changesSize.insert(Scene::MaxEdgeSlope);
    addSpinBox(Scene::EdgeThickness, "Edge &thickness", 1.0, 10.0);
    changesSize.insert(Scene::EdgeThickness);

    addSpinBox(Scene::FontSize, "&Font size", 5.0, 32.0);
    changesLabel.insert(Scene::FontSize);

    addSpinBox(Scene::EdgeSaturation, "Edge color saturation", 0.0, 1.0);
    changesColor.insert(Scene::EdgeSaturation);
    addSpinBox(Scene::EdgeValue, "Edge color value", 0.0, 1.0);
    changesColor.insert(Scene::EdgeValue);

    addSpinBox(Scene::TextSaturation, "Text color saturation", 0.0, 1.0);
    changesLabel.insert(Scene::TextSaturation);
    addSpinBox(Scene::TextValue, "Text color value", 0.0, 1.0);
    changesLabel.insert(Scene::TextValue);

    addSpinBox(Scene::AdditionalNodeSaturation,
               "Referenced node color saturation", 0.0, 1.0);
    changesColor.insert(Scene::AdditionalNodeSaturation);
    addSpinBox(Scene::AdditionalNodeValue,
               "Referenced node color value", 0.0, 1.0);
    changesColor.insert(Scene::AdditionalNodeValue);

    addSpinBox(Scene::LabelPlacementTime, "Label placement timeout", 0.0, 1.0);
    addSpinBox(Scene::AbsoluteCoordsTime, "Timeout for for force-based algo",
               0.0, 10.0);

    addSpinBox(Scene::YearFontSize, "Font size for year labels", 5.0, 32.0);
    changesColor.insert(Scene::YearFontSize);
    addSpinBox(Scene::YearLineAlpha, "Year labels/lines opacity", 0.0, 1.0);
    changesColor.insert(Scene::YearLineAlpha);
    addSpinBox(Scene::YearLineWidth, "Year lines width", 0.1, 10.0);
    changesColor.insert(Scene::YearLineWidth);

    addSpinBox(Scene::AnimationDuration, "&Animation duration", 0.1, 5.0);
}

void VisualisationSettingsWidget::updateSceneParameters()
{
    bool changedColor = false, changedLabels = false, changedSize = false;

    for (int i = 0; i < Scene::NParameters; i++) {
        if (!spinBox[i]) {
            continue;
        }
        if (!sender() || sender() == spinBox[i]) {
            auto value = static_cast<qreal>(spinBox[i]->value());
            if (scene->parameters[i] != value) {
                changedColor = changedColor || changesColor.contains(i);
                changedLabels = changedLabels || changesLabel.contains(i);
                changedSize = changedSize || changesSize.contains(i);
            }
            scene->parameters[i] = value;
        }
    }

    if (blockUpdates) {
        return;
    }

    if (changedSize) {
        scene->absoluteCoords();
        return;
    }

    if (changedColor) {
        scene->build();
    }

    if (changedLabels) {
        scene->placeLabels();
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
    scene->absoluteCoords();
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
