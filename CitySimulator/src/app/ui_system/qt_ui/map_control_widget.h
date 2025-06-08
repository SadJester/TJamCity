#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QListWidget>

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

			void setVehicles(VehicleAnalyzeWidget* vehiclesWidget) { _vehiclesWidget = vehiclesWidget; }

		private:
			void UpdateButtonsState();
			void UpdateLabels();
			bool openFile(std::string_view fileName);

			void createVehicleInformation(QVBoxLayout* layout);
			void createLayerSelection(QVBoxLayout* layout);

		private slots:
			void onZoomIn();
			void onZoomOut();
			void moveNorth();
			void moveSouth();
			void moveWest();
			void moveEast();
			void onUpdate();
			void openOSMFile();
			void onLayerSelectionChanged();

		private:
			Application& _application;

			// Temporary button, will erase it after refactoring
			QPushButton* _updateButton = nullptr;

			QPushButton* _zoomInButton = nullptr;
			QPushButton* _zoomOutButton = nullptr;
			QDoubleSpinBox* _latitude = nullptr;
			QDoubleSpinBox* _longitude = nullptr;
			QPushButton* _northButton = nullptr;
			QPushButton* _southButton = nullptr;
			QPushButton* _westButton = nullptr;
			QPushButton* _eastButton = nullptr;
			QPushButton* _openFileButton = nullptr;

			QSpinBox* vehicleCount = nullptr;
			QDoubleSpinBox* vehicleSizeMultipler = nullptr;
			QCheckBox* randomSeed = nullptr;
			QSpinBox* seedValue = nullptr;
			QPushButton* _regenerateVehiclesButton = nullptr;

			QLabel* _zoomLevel = nullptr;
			QListWidget* _layerList = nullptr;

			VehicleAnalyzeWidget* _vehiclesWidget = nullptr;
		};
	} // namespace ui
} // namespace tjs
