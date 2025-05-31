#pragma once

#include <QtWidgets>

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

			void initialize();
		private slots:
			void handleAgentSelection(int index);

		private:
			void updateAgentDetails(const tjs::core::AgentData* agent);

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
			QPushButton* _updateButton;
		};
	} // namespace ui
} // namespace tjs
