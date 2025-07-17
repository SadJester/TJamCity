#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QListWidget>

#include <events/map_events.h>

namespace tjs {
	class Application;

	namespace visualization {
		class MapElement;
	} // namespace visualization

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

		private slots:
			void onUpdate();
			void openOSMFile();
			void onLayerSelectionChanged();

		private:
			Application& _application;

			// Temporary button, will erase it after refactoring
			QPushButton* _updateButton = nullptr;

			QPushButton* _openFileButton = nullptr;

			QSpinBox* vehicleCount = nullptr;
			QDoubleSpinBox* vehicleSizeMultipler = nullptr;
			QDoubleSpinBox* simplifiedThreshold = nullptr;
			QCheckBox* randomSeed = nullptr;
			QSpinBox* seedValue = nullptr;
			QPushButton* _regenerateVehiclesButton = nullptr;

			QLabel* _zoomLevel = nullptr;
			QLabel* _screenCenter = nullptr;
			QLabel* _longtitude = nullptr;
			QListWidget* _layerList = nullptr;
		};
	} // namespace ui
} // namespace tjs
