#pragma once

#include <QtWidgets>

#include <core/events/simulation_events.h>
#include <events/vehicle_events.h>

namespace tjs::model {
	struct VehicleAnalyzeData;
} // namespace tjs::model

namespace tjs::core {
	struct AgentData;
} // namespace tjs::core

namespace tjs {
	class Application;

	namespace ui {
		class VehicleAnalyzeWidget : public QWidget {
		public:
			VehicleAnalyzeWidget(Application& app);
			~VehicleAnalyzeWidget();

			void initialize();
		private slots:
			void handleAgentSelection(int index);

		private:
			void updateAgentDetails(const tjs::core::AgentData* agent);
			void handle_simulation_initialized(const core::events::SimulationInitialized& event);
			void handle_agent_selected(const events::AgentSelected& event);

			Application& _application;
			tjs::model::VehicleAnalyzeData* _model;

			// UI elements
			QComboBox* _agentComboBox;
			QGroupBox* _detailsGroup;
			QLabel* _agentIdValue;
			QLabel* _vehicleIdValue;
			QLabel* _behaviourValue;
			QLabel* _currentGoalValue;
			QLabel* _currentStepGoalValue;
			QLabel* _pathNodeCountValue;
			QTreeWidget* _pathTreeWidget;
		};
	} // namespace ui
} // namespace tjs
