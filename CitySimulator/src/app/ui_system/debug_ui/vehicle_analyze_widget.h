#pragma once

#include <QtWidgets>

#include <core/events/simulation_events.h>
#include <events/project_events.h>
#include <core/events/vehicle_population_events.h>

#include <ui_system/ui_definitions.h>

namespace tjs::model {
	struct VehicleAnalyzeData;
} // namespace tjs::model

namespace tjs::core {
	struct AgentData;
} // namespace tjs::core

namespace tjs {
	class Application;

	namespace ui {
		class VehicleAnalyzeWidget : public QWidget, public ui::ui_listener<VehicleAnalyzeWidget> {
		public:
			VehicleAnalyzeWidget(Application& app);
			~VehicleAnalyzeWidget();

			void initialize();

			void handle(const visualization::visual_sys_lossy_queue::message_t& msg);

		private slots:
			void handleAgentSelection(int index);

		private:
			void updateAgentDetails(const tjs::core::AgentData* agent);

			void handle_simulation_initialized(const core::events::SimulationInitialized& event);
			void handle_population(const core::events::VehiclesPopulated& event);
			void handle_open_map(const events::OpenMapEvent& event);

		private:
			Application& _application;
			tjs::model::VehicleAnalyzeData* _model;

			ui::ui_event_bus::handler_t _ui_subs_handler {};

			// UI elements
			QComboBox* _agentComboBox;
			QGroupBox* _detailsGroup;
			QGroupBox* _debugGroup;
			QLabel* _agentIdValue;
			QLabel* _vehicleIdValue;
			QLabel* _behaviourValue;
			QLabel* _currentGoalValue;
			QLabel* _currentStepGoalValue;
			QLabel* _pathNodeCountValue;
			QTreeWidget* _pathTreeWidget;

			QSpinBox* _laneIdSpin;
			QSpinBox* _agentIdSpin;
			QListWidget* _vehicleList;
			QSpinBox* _vehicleSpin;
			QPushButton* _addVehicleButton;
			QPushButton* _removeVehicleButton;
			QComboBox* _breakPhaseCombo;
		};
	} // namespace ui
} // namespace tjs
