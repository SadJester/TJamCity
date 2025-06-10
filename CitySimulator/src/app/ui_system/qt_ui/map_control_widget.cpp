#include "stdafx.h"

#include "ui_system/qt_ui/map_control_widget.h"

#include "Application.h"
#include "settings/general_settings.h"
#include "data/map_renderer_data.h"

#include <QLabel>
#include <QFileDialog>

#include <project/project.h>

// TODO: Dirty hack for now
#include <ui_system/debug_ui/vehicle_analyze_widget.h>

/// TODO: Place somwhere to be more pretty

#include "visualization/Scene.h"
#include "visualization/scene_system.h"
#include "visualization/elements/map_element.h"
#include "data/persistent_render_data.h"

#include <core/data_layer/world_creator.h>
#include <core/simulation/simulation_system.h>
#include <core/store_models/vehicle_analyze_data.h>

namespace tjs {
	namespace ui {
		MapControlWidget::MapControlWidget(Application& application, QWidget* parent)
			: QWidget(parent)
			, _application(application) {
			// Create main layout
			QVBoxLayout* mainLayout = new QVBoxLayout(this);

			_application.message_dispatcher().RegisterHandler(*this, &MapControlWidget::handle_positioning_changed, "MapControlWidget");

			// File button
			_openFileButton = new QPushButton("Open OSMX File");

			// Update button
			_updateButton = new QPushButton("Update");
			mainLayout->addWidget(_updateButton);
			connect(_updateButton, &QPushButton::clicked, this, &MapControlWidget::onUpdate);

			// Create info layout
			QFrame* infoFrame = new QFrame();
			infoFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
			infoFrame->setLineWidth(2);
			infoFrame->setMidLineWidth(1);

			QGridLayout* infoLayout = new QGridLayout(infoFrame);
			_zoomLevel = new QLabel("Meters per pixel: 000", this);
			_zoomLevel->setAlignment(Qt::AlignCenter);
			_zoomLevel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			_zoomLevel->setStyleSheet("font-size: 12px; font-weight: bold;");
			infoLayout->addWidget(_zoomLevel, 0, 0);

			// Create coordinates layout
			QHBoxLayout* coordsLayout = new QHBoxLayout(infoFrame);
			_projectCenter = new QLabel("Center: 000, 000", this);
			_projectCenter->setAlignment(Qt::AlignCenter);
			_projectCenter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			_projectCenter->setStyleSheet("font-size: 12px; font-weight: bold;");
			infoLayout->addWidget(_projectCenter, 1, 0);

			// Add layer selection
			createLayerSelection(mainLayout);

			// Add vehicle information
			createVehicleInformation(mainLayout);

			// Add all elements to main layout
			mainLayout->addWidget(_openFileButton);
			mainLayout->addWidget(infoFrame);
			mainLayout->addLayout(coordsLayout);

			// Connections
			connect(_openFileButton, &QPushButton::clicked, this, &MapControlWidget::openOSMFile);
			UpdateButtonsState();
		}

		MapControlWidget::~MapControlWidget() {
			_application.message_dispatcher().UnregisterHandler<events::MapPositioningChanged>("MapControlWidget");
		}

		void MapControlWidget::createVehicleInformation(QVBoxLayout* layout) {
			QFrame* infoFrame = new QFrame();
			infoFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
			infoFrame->setLineWidth(2);
			infoFrame->setMidLineWidth(1);

			QVBoxLayout* mainLayout = new QVBoxLayout(infoFrame);

			// Vehicles count
			QHBoxLayout* intLayout = new QHBoxLayout();
			QLabel* intLabel = new QLabel("Vehicles count:", this);
			vehicleCount = new QSpinBox(this);
			vehicleCount->setRange(1, 1000);
			vehicleCount->setValue(_application.settings().simulationSettings.vehiclesCount);
			intLayout->addWidget(intLabel);
			intLayout->addWidget(vehicleCount);
			mainLayout->addLayout(intLayout);

			// Float value
			QHBoxLayout* floatLayout = new QHBoxLayout();
			QLabel* floatLabel = new QLabel("Float Value:", this);
			vehicleSizeMultipler = new QDoubleSpinBox(this);
			vehicleSizeMultipler->setRange(1.0f, 50.0f);
			vehicleSizeMultipler->setValue(_application.settings().render.vehicleScaler);
			vehicleSizeMultipler->setSingleStep(0.5);
			vehicleSizeMultipler->setDecimals(1);
			floatLayout->addWidget(floatLabel);
			floatLayout->addWidget(vehicleSizeMultipler);
			mainLayout->addLayout(floatLayout);

			// Random seed checkbox and spinbox
			randomSeed = new QCheckBox(tr("Random Seed"), this);
			randomSeed->setChecked(_application.settings().simulationSettings.randomSeed);
			mainLayout->addWidget(randomSeed);

			seedValue = new QSpinBox(this);
			seedValue->setRange(0, 99999);
			seedValue->setValue(_application.settings().simulationSettings.seedValue);
			seedValue->setVisible(!_application.settings().simulationSettings.randomSeed);
			mainLayout->addWidget(seedValue);

			_regenerateVehiclesButton = new QPushButton("Regenerate vehicles", this);
			mainLayout->addWidget(_regenerateVehiclesButton);

			// Подключения сигналов
			connect(vehicleCount, &QSpinBox::valueChanged, [this](int value) {
				_application.settings().simulationSettings.vehiclesCount = value;
			});

			connect(vehicleSizeMultipler, &QDoubleSpinBox::valueChanged, [this](double value) {
				_application.settings().render.vehicleScaler = value;
			});

			connect(randomSeed, &QCheckBox::checkStateChanged, [this](int state) {
				_application.settings().simulationSettings.randomSeed = state == Qt::Checked;
				seedValue->setVisible(state != Qt::Checked); // Показываем/скрываем spinbox
			});

			connect(seedValue, &QSpinBox::valueChanged, [this](int value) {
				_application.settings().simulationSettings.seedValue = value;
			});

			connect(_regenerateVehiclesButton, &QPushButton::clicked, [this]() {
				tjs::core::WorldCreator::createRandomVehicles(_application.worldData(), _application.settings().simulationSettings);
				_application.simulationSystem().initialize();
				_application.stores().get_model<core::model::VehicleAnalyzeData>()->agent = nullptr;
			});

			layout->addWidget(infoFrame);
		}

		void MapControlWidget::createLayerSelection(QVBoxLayout* layout) {
			QFrame* layerFrame = new QFrame();
			layerFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
			layerFrame->setLineWidth(2);
			layerFrame->setMidLineWidth(1);

			QVBoxLayout* layerLayout = new QVBoxLayout(layerFrame);
			QLabel* layerLabel = new QLabel("Map Layers:", this);
			layerLayout->addWidget(layerLabel);

			_layerList = new QListWidget(this);
			_layerList->setSelectionMode(QAbstractItemView::MultiSelection);

			// Add layer options
			QListWidgetItem* waysItem = new QListWidgetItem("Roads", _layerList);
			waysItem->setData(Qt::UserRole, static_cast<uint32_t>(core::model::MapRendererLayer::Ways));
			waysItem->setSelected(true);

			QListWidgetItem* nodesItem = new QListWidgetItem("Nodes", _layerList);
			nodesItem->setData(Qt::UserRole, static_cast<uint32_t>(core::model::MapRendererLayer::Nodes));

			QListWidgetItem* trafficItem = new QListWidgetItem("Traffic Lights", _layerList);
			trafficItem->setData(Qt::UserRole, static_cast<uint32_t>(core::model::MapRendererLayer::TrafficLights));

			QListWidgetItem* networkItem = new QListWidgetItem("Network Graph", _layerList);
			networkItem->setData(Qt::UserRole, static_cast<uint32_t>(core::model::MapRendererLayer::NetworkGraph));

			// Set fixed height based on number of items
			const int itemHeight = _layerList->sizeHintForRow(0);
			const int totalItems = _layerList->count();
			const int spacing = _layerList->spacing();
			const int frameWidth = _layerList->frameWidth();
			const int fixedHeight = (itemHeight * totalItems) + (spacing * (totalItems - 1)) + (frameWidth * 2);
			_layerList->setFixedHeight(fixedHeight);
			_layerList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

			layerLayout->addWidget(_layerList);
			layout->addWidget(layerFrame);

			connect(_layerList, &QListWidget::itemSelectionChanged, this, &MapControlWidget::onLayerSelectionChanged);
		}

		void MapControlWidget::onLayerSelectionChanged() {
			core::model::MapRendererLayer layers = core::model::MapRendererLayer::None;
			for (int i = 0; i < _layerList->count(); ++i) {
				QListWidgetItem* item = _layerList->item(i);
				if (item->isSelected()) {
					layers = layers | static_cast<core::model::MapRendererLayer>(item->data(Qt::UserRole).toUInt());
				}
			}

			auto renderData = _application.stores().get_model<core::model::MapRendererData>();
			if (renderData) {
				renderData->visibleLayers = layers;
			}
		}

		void MapControlWidget::UpdateButtonsState() {
			const bool value = !_application.settings().general.selectedFile.empty();
		}

		void MapControlWidget::UpdateLabels() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			_zoomLevel->setText(QString("Meters per pixel: %1").arg(render_data->metersPerPixel));
			_projectCenter->setText(QString("Center: %1, %2").arg(render_data->projectionCenter.latitude).arg(render_data->projectionCenter.longitude));

			_application.settings().general.zoomLevel = render_data->metersPerPixel;
		}

		void MapControlWidget::onUpdate() {
			if (_layerList == nullptr) {
				return;
			}

			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			// Update layer selection based on current state
			for (int i = 0; i < _layerList->count(); ++i) {
				QListWidgetItem* item = _layerList->item(i);
				core::model::MapRendererLayer layer = static_cast<core::model::MapRendererLayer>(item->data(Qt::UserRole).toUInt());
				item->setSelected(static_cast<uint32_t>(render_data->visibleLayers & layer) != 0);
			}
			UpdateLabels();
		}

		void MapControlWidget::handle_positioning_changed(const events::MapPositioningChanged& event) {
			UpdateLabels();
		}

		void MapControlWidget::openOSMFile() {
			QString fileName = QFileDialog::getOpenFileName(
				this,
				tr("Open OSMX File"),
				"",
				tr("OSMX Files (*.osmx)"));

			const auto file_name = fileName.toStdString();
			if (file_name.empty()) {
				return;
			}

			if (tjs::open_map(file_name, _application)) {
				_application.settings().general.selectedFile = file_name;
				onUpdate();
			}
		}
	} // namespace ui
} // namespace tjs
