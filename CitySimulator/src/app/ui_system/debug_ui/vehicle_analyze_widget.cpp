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

		// Agent selection combo box
		QHBoxLayout* selectionLayout = new QHBoxLayout();
		QLabel* agentLabel = new QLabel("Select Agent:", this);
		_agentComboBox = new QComboBox(this);
		setupAgentCombo();

		selectionLayout->addWidget(agentLabel);
		selectionLayout->addWidget(_agentComboBox, 1);
		mainLayout->addLayout(selectionLayout);

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
		mainLayout->addWidget(_detailsGroup);
		_detailsGroup->setVisible(false);

		// Connect signals
		connect(_agentComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &VehicleAnalyzeWidget::handleAgentSelection);

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
