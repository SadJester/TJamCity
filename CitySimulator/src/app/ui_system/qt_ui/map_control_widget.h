#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QListWidget>
#include <QComboBox>
#include <QScrollArea>

#include <events/map_events.h>
#include <core/events/vehicle_population_events.h>

namespace tjs {
	class Application;

	namespace visualization {
		class MapElement;
	} // namespace visualization

	namespace core::simulation {
		struct AgentTask;
	} // namespace core::simulation

	namespace ui {
		class VehicleAnalyzeWidget;

		class MapControlWidget : public QWidget {
			//Q_OBJECT

		public:
			explicit MapControlWidget(Application& application, QWidget* parent = nullptr);
			~MapControlWidget() override;

		private:
			void UpdateButtonsState();
			void UpdateLabels();
			void handle_positioning_changed(const events::MapPositioningChanged& event);

			void createVehicleInformation(QVBoxLayout* layout);
			void createLayerSelection(QVBoxLayout* layout);
			void handle_population(const core::events::VehiclesPopulated& event);

		private slots:
			void onUpdate();
			void openOSMFile();
			void onLayerSelectionChanged();

		private:
			Application& _application;

			// Temporary button, will erase it after refactoring
			QPushButton* _updateButton = nullptr;

			QPushButton* _openFileButton = nullptr;

			QComboBox* _generatorTypeCombo = nullptr;
			QWidget* _vehicleCountWidget = nullptr;
			QWidget* _flowWidget = nullptr;
			QSpinBox* vehicleCount = nullptr;
			QScrollArea* _spawn_scroll = nullptr;
			QWidget* _spawn_widget = nullptr;
			QVBoxLayout* _spawn_layout = nullptr;
			QPushButton* _add_spawn_button = nullptr;
			QDoubleSpinBox* vehicleSizeMultipler = nullptr;
			QDoubleSpinBox* simplifiedThreshold = nullptr;
			QCheckBox* randomSeed = nullptr;
			QSpinBox* seedValue = nullptr;
			QComboBox* _movementAlgoCombo = nullptr;
			QPushButton* _regenerateVehiclesButton = nullptr;

			QLabel* _zoomLevel = nullptr;
			QLabel* _screenCenter = nullptr;
			QLabel* _longtitude = nullptr;
			QListWidget* _layerList = nullptr;

			QLabel* _populationLabel = nullptr;

			struct SpawnRow {
				QWidget* container = nullptr;
				QSpinBox* lane = nullptr;
				QSpinBox* vehicles_per_hour = nullptr;
				QComboBox* goal_selection = nullptr;
				QSpinBox* goal_node_id = nullptr;
				QSpinBox* max_vehicles = nullptr;
			};
			std::vector<SpawnRow> _spawn_rows;

			void add_spawn_row(const core::simulation::AgentTask& task);
		};
	} // namespace ui
} // namespace tjs
