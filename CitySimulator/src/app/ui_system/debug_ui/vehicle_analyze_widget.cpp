#include <stdafx.h>

#include <ui_system/debug_ui/vehicle_analyze_widget.h>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>

#include <Application.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/agent/agent_data.h>
#include <core/simulation/simulation_system.h>
#include <core/simulation/simulation_debug.h>
#include <events/vehicle_events.h>

namespace tjs::ui {
	VehicleAnalyzeWidget::VehicleAnalyzeWidget(Application& app)
		: _application(app)
		, _agentComboBox(nullptr)
		, _detailsGroup(nullptr)
		, _pathTreeWidget(nullptr) {
		initialize();

		_application.simulationSystem().message_dispatcher().register_handler(*this, &VehicleAnalyzeWidget::handle_simulation_initialized, "VehicleAnalyzeWidget");
		_application.message_dispatcher().register_handler(*this, &VehicleAnalyzeWidget::handle_agent_selected, "VehicleAnalyzeWidget");
		_application.message_dispatcher().register_handler(*this, &VehicleAnalyzeWidget::handle_open_map, "VehicleAnalyzeWidget");
	}

	VehicleAnalyzeWidget::~VehicleAnalyzeWidget() {
		_application.simulationSystem().message_dispatcher().unregister_handler<core::events::SimulationInitialized>("VehicleAnalyzeWidget");
		_application.message_dispatcher().unregister_handler<events::AgentSelected>("VehicleAnalyzeWidget");
		_application.message_dispatcher().unregister_handler<events::OpenMapEvent>("VehicleAnalyzeWidget");
	}

	void VehicleAnalyzeWidget::handle_simulation_initialized(const core::events::SimulationInitialized& event) {
		_application.stores().get_entry<core::model::VehicleAnalyzeData>()->agent = nullptr;
		initialize();
	}

	void VehicleAnalyzeWidget::handle_open_map(const events::OpenMapEvent& event) {
		_application.stores().get_entry<core::model::VehicleAnalyzeData>()->agent = nullptr;
		initialize();
	}

	void VehicleAnalyzeWidget::handle_agent_selected(const events::AgentSelected& event) {
		core::model::VehicleAnalyzeData* model = _application.stores().get_entry<core::model::VehicleAnalyzeData>();
		model->set_agent(event.agent);
		if (event.agent) {
			// Update combo box to selected agent
			for (int i = 0; i < _agentComboBox->count(); ++i) {
				if (_agentComboBox->itemData(i).value<uint64_t>() == event.agent->id) {
					_agentComboBox->setCurrentIndex(i);
					break;
				}
			}
			updateAgentDetails(event.agent);
			_detailsGroup->setVisible(true);
		} else {
			_agentComboBox->setCurrentIndex(0);
			_detailsGroup->setVisible(false);
		}
	}

	void VehicleAnalyzeWidget::initialize() {
		auto& simulation_system = _application.simulationSystem();
		const auto& agents = simulation_system.agents();

		auto setupAgentCombo = [this, &agents]() {
			_agentComboBox->addItem("-- None --", QVariant::fromValue<uint64_t>(0));

			for (const core::AgentData& agent : agents) {
				_agentComboBox->addItem(
					"Agent " + QString::number(agent.id),
					QVariant::fromValue(agent.id));
			}

			if (agents.size() == 1) {
				_agentComboBox->setCurrentIndex(1);
			}
		};

		if (_agentComboBox != nullptr) {
			_agentComboBox->clear();
			setupAgentCombo();
			return;
		}

		QVBoxLayout* mainLayout = new QVBoxLayout(this);

		_debugGroup = new QGroupBox("Simulation Debug", this);
		QVBoxLayout* debugLayout = new QVBoxLayout(_debugGroup);

		// Agent selection combo box
		QHBoxLayout* selectionLayout = new QHBoxLayout();
		QLabel* agentLabel = new QLabel("Select Agent:", this);
		_agentComboBox = new QComboBox(this);
		setupAgentCombo();

		selectionLayout->addWidget(agentLabel);
		selectionLayout->addWidget(_agentComboBox, 1);
		debugLayout->addLayout(selectionLayout);

		// Agent details group
		_detailsGroup = new QGroupBox("Agent Details", this);
		QFormLayout* formLayout = new QFormLayout(_detailsGroup);

		_agentIdValue = new QLabel(_detailsGroup);
		_vehicleIdValue = new QLabel(_detailsGroup);
		_behaviourValue = new QLabel(_detailsGroup);
		_currentGoalValue = new QLabel(_detailsGroup);
		_currentStepGoalValue = new QLabel(_detailsGroup);
		_pathNodeCountValue = new QLabel(_detailsGroup);
		_pathTreeWidget = new QTreeWidget(_detailsGroup);
		_pathTreeWidget->setHeaderHidden(true);
		QTreeWidgetItem* rootItem = new QTreeWidgetItem(_pathTreeWidget);
		rootItem->setText(0, "Path Nodes");
		rootItem->setExpanded(false);

		formLayout->addRow("Agent ID:", _agentIdValue);
		formLayout->addRow("Vehicle ID:", _vehicleIdValue);
		formLayout->addRow("Behaviour:", _behaviourValue);
		formLayout->addRow("Current Goal Node:", _currentGoalValue);
		formLayout->addRow("Current Step Goal:", _currentStepGoalValue);
		formLayout->addRow("Path Nodes:", _pathNodeCountValue);
		formLayout->addRow("", _pathTreeWidget);

		_detailsGroup->setLayout(formLayout);
		debugLayout->addWidget(_detailsGroup);
		_detailsGroup->setVisible(false);

		// Debug controls
		QFormLayout* debugForm = new QFormLayout();

		_laneIdSpin = new QSpinBox(_debugGroup);
		_laneIdSpin->setRange(0, 1000000);
		_laneIdSpin->setValue(_application.settings().simulationSettings.debug_data.lane_id);
		debugForm->addRow("Lane ID:", _laneIdSpin);

		_agentIdSpin = new QSpinBox(_debugGroup);
		_agentIdSpin->setRange(0, 1000000);
		_agentIdSpin->setValue(_application.settings().simulationSettings.debug_data.lane_id);
		debugForm->addRow("Agent ID:", _agentIdSpin);

		_vehicleList = new QListWidget(_debugGroup);
		_vehicleList->setDragDropMode(QAbstractItemView::InternalMove);
		_vehicleList->setSelectionMode(QAbstractItemView::SingleSelection);
		_vehicleList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
		for (uint64_t id : _application.settings().simulationSettings.debug_data.vehicle_indices) {
			_vehicleList->addItem(QString::number(id));
		}
		debugForm->addRow("Vehicles:", _vehicleList);

		QHBoxLayout* vehicleControls = new QHBoxLayout();
		_vehicleSpin = new QSpinBox(_debugGroup);
		_vehicleSpin->setRange(0, 1000000);
		_addVehicleButton = new QPushButton("Add", _debugGroup);
		_removeVehicleButton = new QPushButton("Remove", _debugGroup);
		vehicleControls->addWidget(_vehicleSpin);
		vehicleControls->addWidget(_addVehicleButton);
		vehicleControls->addWidget(_removeVehicleButton);
		debugForm->addRow("", vehicleControls);

		_breakPhaseCombo = new QComboBox(_debugGroup);
		_breakPhaseCombo->addItem("None", static_cast<int>(core::simulation::SimulationMovementPhase::None));
		_breakPhaseCombo->addItem("IDM_Phase1_Lane", static_cast<int>(core::simulation::SimulationMovementPhase::IDM_Phase1_Lane));
		_breakPhaseCombo->addItem("IDM_Phase1_Vehicle", static_cast<int>(core::simulation::SimulationMovementPhase::IDM_Phase1_Vehicle));
		_breakPhaseCombo->addItem("IDM_Phase2_Agent", static_cast<int>(core::simulation::SimulationMovementPhase::IDM_Phase2_Agent));
		_breakPhaseCombo->addItem("IDM_Phase2_ChooseLane", static_cast<int>(core::simulation::SimulationMovementPhase::IDM_Phase2_ChooseLane));
		_breakPhaseCombo->setCurrentIndex(static_cast<int>(_application.settings().simulationSettings.debug_data.movement_phase));
		debugForm->addRow("Break movement phase:", _breakPhaseCombo);

		debugLayout->addLayout(debugForm);

		_debugGroup->setLayout(debugLayout);
		mainLayout->addWidget(_debugGroup);

		// Connect signals
		connect(_agentComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &VehicleAnalyzeWidget::handleAgentSelection);

		auto updateVehicles = [this]() {
			auto& vec = _application.settings().simulationSettings.debug_data.vehicle_indices;
			vec.clear();
			for (int i = 0; i < _vehicleList->count(); ++i) {
				vec.push_back(_vehicleList->item(i)->text().toULongLong());
			}
		};

		connect(_laneIdSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
			_application.settings().simulationSettings.debug_data.lane_id = value;
		});

		connect(_agentIdSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
			_application.settings().simulationSettings.debug_data.agent_id = value;
		});

		connect(_addVehicleButton, &QPushButton::clicked, [this, updateVehicles]() mutable {
			_vehicleList->addItem(QString::number(_vehicleSpin->value()));
			updateVehicles();
		});

		connect(_removeVehicleButton, &QPushButton::clicked, [this, updateVehicles]() mutable {
			auto* item = _vehicleList->currentItem();
			if (item) {
				delete item;
				updateVehicles();
			}
		});

		connect(_vehicleList->model(), &QAbstractItemModel::rowsMoved, [updateVehicles](const QModelIndex&, int, int, const QModelIndex&, int) { updateVehicles(); });
		connect(_vehicleList, &QListWidget::itemChanged, [updateVehicles](QListWidgetItem*) { updateVehicles(); });

		connect(_breakPhaseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			_application.settings().simulationSettings.debug_data.movement_phase = static_cast<core::simulation::SimulationMovementPhase>(index);
		});

		setLayout(mainLayout);
	}

	void VehicleAnalyzeWidget::handleAgentSelection(int index) {
		core::model::VehicleAnalyzeData* model = _application.stores().get_entry<core::model::VehicleAnalyzeData>();

		if (index <= 0) { // "-- None --" selected
			model->set_agent(nullptr);
			_detailsGroup->setVisible(false);
			return;
		}

		uint64_t agentId = _agentComboBox->currentData().value<uint64_t>();
		auto& agents = _application.simulationSystem().agents();

		auto it = std::find_if(agents.begin(), agents.end(),
			[agentId](const core::AgentData& a) { return a.id == agentId; });

		if (it != agents.end()) {
			model->set_agent(&(*it));
			updateAgentDetails(&(*it));
			_detailsGroup->setVisible(true);
		}
	}

	void VehicleAnalyzeWidget::updateAgentDetails(const core::AgentData* agent) {
		if (!agent) {
			return;
		}

		_agentIdValue->setText(QString::number(agent->id));

		if (agent->vehicle) {
			_vehicleIdValue->setText(QString::number(agent->vehicle->uid));
		} else {
			_vehicleIdValue->setText("N/A");
		}

		// Convert behaviour to string
		QString behaviourStr;
		switch (agent->behaviour) {
			case core::TacticalBehaviour::Normal:
				behaviourStr = "Normal";
				break;
			case core::TacticalBehaviour::Aggressive:
				behaviourStr = "Aggressive";
				break;
			case core::TacticalBehaviour::Defensive:
				behaviourStr = "Defensive";
				break;
			case core::TacticalBehaviour::Emergency:
				behaviourStr = "Emergency";
				break;
			default:
				behaviourStr = "Unknown";
		}
		_behaviourValue->setText(behaviourStr);

		_currentGoalValue->setText(agent->currentGoal ?
									   QString::number(agent->currentGoal->uid) :
									   "None");

		if (agent->currentGoal) {
			auto& coordinates = agent->currentGoal->coordinates;
			_currentStepGoalValue->setText(
				QString("(%1, %2)").arg(coordinates.latitude).arg(coordinates.longitude));
		}

		_pathNodeCountValue->setText(QString::number(agent->path.size()));
		_pathTreeWidget->clear();
		QTreeWidgetItem* rootItem = new QTreeWidgetItem(_pathTreeWidget);
		rootItem->setText(0, "Path Nodes");
		rootItem->setExpanded(false);
		for (const core::Edge* edge : agent->path) {
			if (!edge || !edge->end_node) {
				continue;
			}
			QTreeWidgetItem* item = new QTreeWidgetItem();
			item->setText(0, QString::number(edge->end_node->uid));
			rootItem->addChild(item);
		}
	}
} // namespace tjs::ui
