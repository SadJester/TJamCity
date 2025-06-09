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
			QCheckBox* randomSeed = nullptr;
			QSpinBox* seedValue = nullptr;
			QPushButton* _regenerateVehiclesButton = nullptr;

			QLabel* _zoomLevel = nullptr;
			QLabel* _projectCenter = nullptr;
			QLabel* _longtitude = nullptr;
			QListWidget* _layerList = nullptr;

			VehicleAnalyzeWidget* _vehiclesWidget = nullptr;
		};
	} // namespace ui
} // namespace tjs
