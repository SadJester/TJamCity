#include <stdafx.h>

#include <ui_system/qt_ui/map_control_widget.h>

#include <Application.h>
#include <settings/general_settings.h>
#include <data/map_renderer_data.h>

#include <QLabel>
#include <QFileDialog>
#include <QComboBox>
#include <QScrollArea>

#include <project/project.h>

/// TODO: Place somwhere to be more pretty

#include <visualization/Scene.h>
#include <visualization/scene_system.h>
#include <visualization/elements/map_element.h>
#include <data/persistent_render_data.h>

#include <core/data_layer/world_creator.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/simulation_settings.h>
#include <core/simulation/agent/agent_data.h>
#include <core/store_models/vehicle_analyze_data.h>

namespace tjs {
	namespace ui {
		MapControlWidget::MapControlWidget(Application& application, QWidget* parent)
			: QWidget(parent)
			, _application(application) {
			// Create main layout
			QVBoxLayout* mainLayout = new QVBoxLayout(this);

			_application.message_dispatcher().register_handler(*this, &MapControlWidget::handle_positioning_changed, "MapControlWidget");
			_application.simulationSystem().message_dispatcher().register_handler(*this, &MapControlWidget::handle_population, "MapControlWidget");

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
			_screenCenter = new QLabel("Center: 000, 000", this);
			_screenCenter->setAlignment(Qt::AlignCenter);
			_screenCenter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			_screenCenter->setStyleSheet("font-size: 12px; font-weight: bold;");
			infoLayout->addWidget(_screenCenter, 1, 0);

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
			_application.message_dispatcher().unregister_handler<events::MapPositioningChanged>("MapControlWidget");
			_application.simulationSystem().message_dispatcher().unregister_handler<core::events::VehiclesPopulated>("MapControlWidget");
		}

		void MapControlWidget::createVehicleInformation(QVBoxLayout* layout) {
			QFrame* infoFrame = new QFrame();
			infoFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
			infoFrame->setLineWidth(2);
			infoFrame->setMidLineWidth(1);

			QVBoxLayout* mainLayout = new QVBoxLayout(infoFrame);

			// Create generator algorithm frame
			QFrame* generatorFrame = new QFrame();
			generatorFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
			generatorFrame->setLineWidth(1);
			generatorFrame->setMidLineWidth(1);

			QVBoxLayout* generatorLayout = new QVBoxLayout(generatorFrame);
			QLabel* generatorTitleLabel = new QLabel("Generator Algorithm", this);
			generatorTitleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
			generatorLayout->addWidget(generatorTitleLabel);

			QHBoxLayout* generatorTypeLayout = new QHBoxLayout();
			QLabel* generatorLabel = new QLabel("Generator:", this);
			_generatorTypeCombo = new QComboBox(this);
			_generatorTypeCombo->addItem(
				"Bulk",
				static_cast<int>(core::simulation::GeneratorType::Bulk));
			_generatorTypeCombo->addItem(
				"Flow",
				static_cast<int>(core::simulation::GeneratorType::Flow));
			_generatorTypeCombo->setCurrentIndex(
				static_cast<int>(_application.settings().simulationSettings.generator_type));
			generatorTypeLayout->addWidget(generatorLabel);
			generatorTypeLayout->addWidget(_generatorTypeCombo);
			generatorLayout->addLayout(generatorTypeLayout);

			_vehicleCountWidget = new QWidget(this);
			QHBoxLayout* intLayout = new QHBoxLayout(_vehicleCountWidget);
			QLabel* intLabel = new QLabel("Vehicles count:", this);
			vehicleCount = new QSpinBox(this);
			vehicleCount->setRange(1, 10000000);
			vehicleCount->setValue(_application.settings().simulationSettings.vehiclesCount);
			intLayout->addWidget(intLabel);
			intLayout->addWidget(vehicleCount);
			generatorLayout->addWidget(_vehicleCountWidget);

			_flowWidget = new QWidget(this);
			QVBoxLayout* flowLayout = new QVBoxLayout(_flowWidget);
			auto& spawns = _application.settings().simulationSettings.spawn_requests;
			if (spawns.empty()) {
				spawns.push_back({});
			}
			_spawn_scroll = new QScrollArea(this);
			_spawn_scroll->setWidgetResizable(true);
			_spawn_scroll->setFixedHeight(150);
			_spawn_widget = new QWidget();
			_spawn_layout = new QVBoxLayout(_spawn_widget);
			_spawn_layout->setAlignment(Qt::AlignTop);
			_spawn_scroll->setWidget(_spawn_widget);
			flowLayout->addWidget(_spawn_scroll);

			for (const auto& task : spawns) {
				add_spawn_row(task);
			}

			_add_spawn_button = new QPushButton("+", this);
			flowLayout->addWidget(_add_spawn_button);
			generatorLayout->addWidget(_flowWidget);

			// Add generator frame to main layout
			mainLayout->addWidget(generatorFrame);

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

			QHBoxLayout* simplifiedLayout = new QHBoxLayout();
			QLabel* simplifiedLabel = new QLabel("Simplified MPP:", this);
			simplifiedThreshold = new QDoubleSpinBox(this);
			simplifiedThreshold->setRange(1.0, 100.0);
			simplifiedThreshold->setSingleStep(1.0);
			if (auto* renderData = _application.stores().get_entry<core::model::MapRendererData>()) {
				simplifiedThreshold->setValue(renderData->simplifiedViewThreshold);
			}
			simplifiedLayout->addWidget(simplifiedLabel);
			simplifiedLayout->addWidget(simplifiedThreshold);
			mainLayout->addLayout(simplifiedLayout);

			// Random seed checkbox and spinbox
			randomSeed = new QCheckBox(tr("Random Seed"), this);
			randomSeed->setChecked(_application.settings().simulationSettings.randomSeed);
			mainLayout->addWidget(randomSeed);

			seedValue = new QSpinBox(this);
			seedValue->setRange(0, 99999);
			seedValue->setValue(_application.settings().simulationSettings.seedValue);
			seedValue->setVisible(!_application.settings().simulationSettings.randomSeed);
			mainLayout->addWidget(seedValue);

			QHBoxLayout* algoLayout = new QHBoxLayout();
			QLabel* algoLabel = new QLabel("Movement Algo:", this);
			_movementAlgoCombo = new QComboBox(this);
			_movementAlgoCombo->addItem("Agent", static_cast<int>(core::MovementAlgoType::Agent));
			_movementAlgoCombo->addItem("IDM", static_cast<int>(core::MovementAlgoType::IDM));
			_movementAlgoCombo->setCurrentIndex(static_cast<int>(_application.settings().simulationSettings.movement_algo));
			algoLayout->addWidget(algoLabel);
			algoLayout->addWidget(_movementAlgoCombo);
			mainLayout->addLayout(algoLayout);

			_regenerateVehiclesButton = new QPushButton("Regenerate vehicles", this);
			mainLayout->addWidget(_regenerateVehiclesButton);

			_populationLabel = new QLabel("", this);
			_populationLabel->setStyleSheet("color: blue;");
			mainLayout->addWidget(_populationLabel);

			auto type = _application.settings().simulationSettings.generator_type;
			_vehicleCountWidget->setVisible(type == core::simulation::GeneratorType::Bulk);
			_flowWidget->setVisible(type == core::simulation::GeneratorType::Flow);

			// Подключения сигналов
			connect(_generatorTypeCombo,
				QOverload<int>::of(&QComboBox::currentIndexChanged),
				[this](int index) {
					auto t = static_cast<core::simulation::GeneratorType>(index);
					_application.settings().simulationSettings.generator_type = t;
					_vehicleCountWidget->setVisible(
						t == core::simulation::GeneratorType::Bulk);
					_flowWidget->setVisible(
						t == core::simulation::GeneratorType::Flow);
				});

			connect(vehicleCount, &QSpinBox::valueChanged, [this](int value) {
				_application.settings().simulationSettings.vehiclesCount = value;
			});

			connect(_add_spawn_button, &QPushButton::clicked, [this]() {
				auto& spawns = _application.settings().simulationSettings.spawn_requests;
				spawns.push_back({});
				add_spawn_row(spawns.back());
			});

			connect(vehicleSizeMultipler, &QDoubleSpinBox::valueChanged, [this](double value) {
				_application.settings().render.vehicleScaler = value;
			});

			connect(simplifiedThreshold, &QDoubleSpinBox::valueChanged, [this](double value) {
				if (auto* renderData = _application.stores().get_entry<core::model::MapRendererData>()) {
					renderData->simplifiedViewThreshold = value;
				}
			});

			connect(randomSeed, &QCheckBox::checkStateChanged, [this](int state) {
				_application.settings().simulationSettings.randomSeed = state == Qt::Checked;
				seedValue->setVisible(state != Qt::Checked); // Показываем/скрываем spinbox
			});

			connect(seedValue, &QSpinBox::valueChanged, [this](int value) {
				_application.settings().simulationSettings.seedValue = value;
			});

			connect(_movementAlgoCombo,
				QOverload<int>::of(&QComboBox::currentIndexChanged),
				[this](int index) {
					_application.settings().simulationSettings.movement_algo =
						static_cast<core::MovementAlgoType>(index);
				});

			connect(_regenerateVehiclesButton, &QPushButton::clicked, [this]() {
				_application.simulationSystem().initialize();
				if (_populationLabel) {
					_populationLabel->clear();
				}
			});

			layout->addWidget(infoFrame);
		}

		void MapControlWidget::add_spawn_row(const core::simulation::AgentTask& task) {
			SpawnRow row;
			row.container = new QWidget(_spawn_widget);
			QHBoxLayout* layout = new QHBoxLayout(row.container);

			QLabel* lane_label = new QLabel("Lane:", this);
			row.lane = new QSpinBox(this);
			row.lane->setRange(0, 100000);
			row.lane->setValue(task.lane_id);

			QLabel* vh_label = new QLabel("Vehicles/hour:", this);
			row.vehicles_per_hour = new QSpinBox(this);
			row.vehicles_per_hour->setRange(0, 100000);
			row.vehicles_per_hour->setValue(task.vehicles_per_hour);

			QLabel* goal_type_label = new QLabel("Goal:", this);
			row.goal_selection = new QComboBox(this);
			row.goal_selection->addItem(
				"Random",
				static_cast<int>(core::AgentGoalSelectionType::RandomSelection));
			row.goal_selection->addItem(
				"Goal node",
				static_cast<int>(core::AgentGoalSelectionType::GoalNodeId));
			row.goal_selection->setCurrentIndex(
				task.goal_selection_type == core::AgentGoalSelectionType::GoalNodeId ? 1 : 0);

			QLabel* goal_id_label = new QLabel("Id:", this);
			row.goal_node_id = new QSpinBox(this);
			row.goal_node_id->setRange(0, std::numeric_limits<int>::max());
			row.goal_node_id->setValue(static_cast<int>(task.goal_node_id));

			QLabel* max_label = new QLabel("Max:", this);
			row.max_vehicles = new QSpinBox(this);
			row.max_vehicles->setRange(0, 1000000);
			row.max_vehicles->setValue(task.max_vehicles);

			QPushButton* remove_button = new QPushButton("-", this);

			layout->addWidget(lane_label);
			layout->addWidget(row.lane);
			layout->addWidget(vh_label);
			layout->addWidget(row.vehicles_per_hour);
			layout->addWidget(goal_type_label);
			layout->addWidget(row.goal_selection);
			layout->addWidget(goal_id_label);
			layout->addWidget(row.goal_node_id);
			layout->addWidget(max_label);
			layout->addWidget(row.max_vehicles);
			layout->addWidget(remove_button);

			if (task.goal_selection_type != core::AgentGoalSelectionType::GoalNodeId) {
				goal_id_label->setVisible(false);
				row.goal_node_id->setVisible(false);
			}

			_spawn_layout->addWidget(row.container);
			_spawn_rows.push_back(row);

			connect(row.lane, &QSpinBox::valueChanged, [this, sp = row.lane](int value) {
				auto& spawns = _application.settings().simulationSettings.spawn_requests;
				for (size_t i = 0; i < _spawn_rows.size(); ++i) {
					if (_spawn_rows[i].lane == sp) {
						spawns[i].lane_id = value;
						break;
					}
				}
			});

			connect(row.vehicles_per_hour, &QSpinBox::valueChanged, [this, sp = row.vehicles_per_hour](int value) {
				auto& spawns = _application.settings().simulationSettings.spawn_requests;
				for (size_t i = 0; i < _spawn_rows.size(); ++i) {
					if (_spawn_rows[i].vehicles_per_hour == sp) {
						spawns[i].vehicles_per_hour = value;
						break;
					}
				}
			});

			connect(row.goal_node_id, &QSpinBox::valueChanged, [this, sp = row.goal_node_id](int value) {
				auto& spawns = _application.settings().simulationSettings.spawn_requests;
				for (size_t i = 0; i < _spawn_rows.size(); ++i) {
					if (_spawn_rows[i].goal_node_id == sp) {
						spawns[i].goal_node_id = static_cast<uint64_t>(value);
						break;
					}
				}
			});

			connect(row.max_vehicles, &QSpinBox::valueChanged, [this, sp = row.max_vehicles](int value) {
				auto& spawns = _application.settings().simulationSettings.spawn_requests;
				for (size_t i = 0; i < _spawn_rows.size(); ++i) {
					if (_spawn_rows[i].max_vehicles == sp) {
						spawns[i].max_vehicles = value;
						break;
					}
				}
			});

			connect(row.goal_selection,
				QOverload<int>::of(&QComboBox::currentIndexChanged),
				[this, combo = row.goal_selection, goal_id_label, goal_spin = row.goal_node_id](int index) {
					bool random = index == 0;
					goal_id_label->setVisible(!random);
					goal_spin->setVisible(!random);
					auto& spawns = _application.settings().simulationSettings.spawn_requests;
					for (size_t i = 0; i < _spawn_rows.size(); ++i) {
						if (_spawn_rows[i].goal_selection == combo) {
							spawns[i].goal_selection_type =
								random ? core::AgentGoalSelectionType::RandomSelection : core::AgentGoalSelectionType::GoalNodeId;
							if (random) {
								spawns[i].goal_node_id = 0;
							}
							break;
						}
					}
				});

			connect(remove_button, &QPushButton::clicked, [this, container = row.container]() {
				auto& spawns = _application.settings().simulationSettings.spawn_requests;
				for (size_t i = 0; i < _spawn_rows.size(); ++i) {
					if (_spawn_rows[i].container == container) {
						_spawn_layout->removeWidget(container);
						container->deleteLater();
						_spawn_rows.erase(_spawn_rows.begin() + static_cast<long>(i));
						if (i < spawns.size()) {
							spawns.erase(spawns.begin() + static_cast<long>(i));
						}
						break;
					}
				}
			});
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

			auto renderData = _application.stores().get_entry<core::model::MapRendererData>();
			if (renderData) {
				renderData->visibleLayers = layers;
			}
		}

		void MapControlWidget::UpdateButtonsState() {
			const bool value = !_application.settings().general.selectedFile.empty();
		}

		void MapControlWidget::UpdateLabels() {
			auto render_data = _application.stores().get_entry<core::model::MapRendererData>();
			if (!render_data) {
				return;
			}

			_zoomLevel->setText(QString("Meters per pixel: %1").arg(render_data->metersPerPixel));
			_screenCenter->setText(QString("Center: %1, %2").arg(render_data->screen_center.x).arg(render_data->screen_center.y));
		}

		void MapControlWidget::onUpdate() {
			if (_layerList == nullptr) {
				return;
			}

			auto render_data = _application.stores().get_entry<core::model::MapRendererData>();
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

		void MapControlWidget::handle_population(const core::events::VehiclesPopulated& event) {
			if (!_populationLabel) {
				return;
			}
			if (event.error) {
				_populationLabel->setText(
					QString("Population error: %1/%2")
						.arg(event.current)
						.arg(event.total));
				_populationLabel->setStyleSheet("color: red;");
				return;
			}

			const auto generator_type = _application.simulationSystem().settings().generator_type;
			if (generator_type == core::simulation::GeneratorType::Bulk && event.current >= event.total) {
				_populationLabel->setText(
					QString("Generated %1 for %2 ticks")
						.arg(event.total)
						.arg(event.creation_ticks));
				_populationLabel->setStyleSheet("color: green;");
			} else {
				_populationLabel->setText(QString("Generation: %2/%3 (%1 in last step)")
						.arg(event.generated)
						.arg(event.current)
						.arg(generator_type == core::simulation::GeneratorType::Bulk ? event.total : 0));
				_populationLabel->setStyleSheet("color: blue;");
			}
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

			if (tjs::open_map_simulation_reinit(file_name, _application)) {
				_application.settings().general.selectedFile = file_name;
				onUpdate();
			}
		}
	} // namespace ui
} // namespace tjs
