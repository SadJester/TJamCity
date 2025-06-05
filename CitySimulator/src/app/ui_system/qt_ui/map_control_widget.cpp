#include "stdafx.h"

#include "ui_system/qt_ui/map_control_widget.h"

#include "Application.h"
#include "settings/general_settings.h"
#include "data/map_renderer_data.h"

#include <QLabel>
#include <QFileDialog>

// TODO: Dirty hack for now
#include <ui_system/debug_ui/vehicle_analyze_widget.h>

/// TODO: Place somwhere to be more pretty
#include "visualization/Scene.h"
#include "visualization/scene_system.h"
#include "visualization/elements/map_element.h"

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

			// File button
			_openFileButton = new QPushButton("Open OSMX File");
			openFile(_application.settings().general.selectedFile);

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
			_zoomLevel->setStyleSheet("font-size: 14px; font-weight: bold;");
			infoLayout->addWidget(_zoomLevel, 0, 0);

			// Create zoom buttons layout
			QHBoxLayout* zoomLayout = new QHBoxLayout();
			_zoomInButton = new QPushButton("Zoom In", this);
			_zoomOutButton = new QPushButton("Zoom Out", this);
			zoomLayout->addWidget(_zoomInButton);
			zoomLayout->addWidget(_zoomOutButton);

			// Create arrows layout
			QFrame* arrowsFrame = new QFrame();
			arrowsFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
			arrowsFrame->setLineWidth(2);
			arrowsFrame->setMidLineWidth(1);

			QGridLayout* arrowsLayout = new QGridLayout(arrowsFrame);

			_northButton = new QPushButton("▲");
			_westButton = new QPushButton("◀");
			_eastButton = new QPushButton("▶");
			_southButton = new QPushButton("▼");

			arrowsLayout->addWidget(_northButton, 0, 1);
			arrowsLayout->addWidget(_westButton, 1, 0);
			arrowsLayout->addWidget(_eastButton, 1, 2);
			arrowsLayout->addWidget(_southButton, 2, 1);

			// Create coordinates layout
			QHBoxLayout* coordsLayout = new QHBoxLayout();
			QLabel* latLabel = new QLabel("Latitude:");
			QLabel* lonLabel = new QLabel("Longitude:");

			_latitude = new QDoubleSpinBox();
			_latitude->setRange(-90.0, 90.0);
			_latitude->setDecimals(6);
			_latitude->setSuffix("°");
			_latitude->setReadOnly(true);

			_longitude = new QDoubleSpinBox();
			_longitude->setRange(-180.0, 180.0);
			_longitude->setDecimals(6);
			_longitude->setSuffix("°");
			_longitude->setReadOnly(true);

			coordsLayout->addWidget(latLabel);
			coordsLayout->addWidget(_latitude);
			coordsLayout->addWidget(lonLabel);
			coordsLayout->addWidget(_longitude);

			// Add layer selection
			createLayerSelection(mainLayout);

			// Add vehicle information
			createVehicleInformation(mainLayout);

			// Add all elements to main layout
			mainLayout->addWidget(_openFileButton);
			mainLayout->addWidget(infoFrame);
			mainLayout->addLayout(zoomLayout);
			mainLayout->addWidget(arrowsFrame);
			mainLayout->addLayout(coordsLayout);

			// Connections
			connect(_zoomInButton, &QPushButton::clicked, this, &MapControlWidget::onZoomIn);
			connect(_zoomOutButton, &QPushButton::clicked, this, &MapControlWidget::onZoomOut);
			connect(_northButton, &QPushButton::clicked, this, &MapControlWidget::moveNorth);
			connect(_southButton, &QPushButton::clicked, this, &MapControlWidget::moveSouth);
			connect(_westButton, &QPushButton::clicked, this, &MapControlWidget::moveWest);
			connect(_eastButton, &QPushButton::clicked, this, &MapControlWidget::moveEast);

			connect(_openFileButton, &QPushButton::clicked, this, &MapControlWidget::openOSMFile);

			connect(_latitude, &QDoubleSpinBox::valueChanged, [this](double value) {
				_application.settings().general.projectionCenter.latitude = value;
			});

			connect(_longitude, &QDoubleSpinBox::valueChanged, [this](double value) {
				_application.settings().general.projectionCenter.longitude = value;
			});

			UpdateButtonsState();
		}

		MapControlWidget::~MapControlWidget() {
			// Cleanup
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
				// TODO: message system
				_application.simulationSystem().initialize();
				_application.stores().get_model<core::model::VehicleAnalyzeData>()->agent = nullptr;
				if (_vehiclesWidget) {
					_vehiclesWidget->initialize();
				}
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
			_zoomInButton->setEnabled(value);
			_zoomOutButton->setEnabled(value);
			_northButton->setEnabled(value);
			_southButton->setEnabled(value);
			_westButton->setEnabled(value);
			_eastButton->setEnabled(value);
			_latitude->setEnabled(value);
			_longitude->setEnabled(value);
		}

		void MapControlWidget::UpdateLabels() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			_zoomLevel->setText(QString("Meters per pixel: %1").arg(render_data->metersPerPixel));
			_latitude->setValue(render_data->projectionCenter.latitude);
			_longitude->setValue(render_data->projectionCenter.longitude);

			_application.settings().general.zoomLevel = render_data->metersPerPixel;
		}

		void MapControlWidget::onZoomIn() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			double currentZoom = render_data->metersPerPixel;
			render_data->set_meters_per_pixel(currentZoom * 0.9); // Zoom in (decrease meters per pixel)
			UpdateLabels();
		}

		void MapControlWidget::onZoomOut() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			double currentZoom = render_data->metersPerPixel;
			render_data->set_meters_per_pixel(currentZoom * 1.1); // Zoom out (increase meters per pixel)
			UpdateLabels();
		}

		double getChangedStep(double metersPerPixel) {
			// Определяем границы
			const double minMetersPerPixel = 0.1;
			const double maxMetersPerPixel = 13.0;

			const double minStep = 0.0000001;
			const double maxStep = 0.001;

			// Линейная интерполяция
			double normalizedValue = (metersPerPixel - minMetersPerPixel) / (maxMetersPerPixel - minMetersPerPixel);

			// Рассчитываем step
			double step = maxStep - (maxStep - minStep) * normalizedValue;

			return step;
		}

		void MapControlWidget::moveNorth() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			core::Coordinates current = render_data->projectionCenter;
			if (current.latitude < 90.0) {
				current.latitude += getChangedStep(render_data->metersPerPixel);
				_latitude->setValue(current.latitude);
				render_data->projectionCenter = current;
			}
		}

		void MapControlWidget::moveSouth() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			core::Coordinates current = render_data->projectionCenter;
			if (current.latitude > -90.0) {
				current.latitude -= getChangedStep(render_data->metersPerPixel);
				_latitude->setValue(current.latitude);
				render_data->projectionCenter = current;
			}
		}

		void MapControlWidget::moveWest() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			core::Coordinates current = render_data->projectionCenter;
			if (current.longitude > -180.0) {
				current.longitude -= getChangedStep(render_data->metersPerPixel);
				_longitude->setValue(current.longitude);
				render_data->projectionCenter = current;
			}
		}

		void MapControlWidget::moveEast() {
			auto render_data = _application.stores().get_model<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			core::Coordinates current = render_data->projectionCenter;
			if (current.longitude < 180.0) {
				current.longitude += getChangedStep(render_data->metersPerPixel);
				_longitude->setValue(current.longitude);
				render_data->projectionCenter = current;
			}
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

			if (const auto& projectionCenter = _application.settings().general.projectionCenter;
				projectionCenter.latitude != 0.0 || projectionCenter.longitude != 0.0) {
				render_data->projectionCenter = _application.settings().general.projectionCenter;
			}
			render_data->metersPerPixel = _application.settings().general.zoomLevel;
			UpdateLabels();

			// Initialize spin boxes with current values
			_latitude->setValue(render_data->projectionCenter.latitude);
			_longitude->setValue(render_data->projectionCenter.longitude);
		}

		bool MapControlWidget::openFile(std::string_view fileName) {
			if (fileName.empty()) {
				return false;
			}
			if (tjs::core::WorldCreator::loadOSMData(_application.worldData(), fileName)) {
				tjs::core::WorldCreator::createRandomVehicles(_application.worldData(), _application.settings().simulationSettings);
				_application.settings().general.selectedFile = fileName;
				onUpdate();

				// TODO: message system
				if (auto scene = _application.sceneSystem().getScene("General"); scene) {
					if (auto mapElement = dynamic_cast<visualization::MapElement*>(scene->getNode("MapElement")); mapElement) {
						mapElement->init();
					}
				}
				return true;
			}
			return false;
		}

		void MapControlWidget::openOSMFile() {
			QString fileName = QFileDialog::getOpenFileName(
				this,
				tr("Open OSMX File"),
				"",
				tr("OSMX Files (*.osmx)"));

			openFile(fileName.toStdString());
		}
	} // namespace ui
} // namespace tjs
