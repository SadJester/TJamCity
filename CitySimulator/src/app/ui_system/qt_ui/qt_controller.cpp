#include "stdafx.h"

#include "ui_system/qt_ui/qt_controller.h"

#include "ui_system/qt_ui/main_window.h"

#include "Application.h"

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>

/// TODO: Place somwhere to be more pretty
#include "ui_system/qt_ui/render_metrics_widget.h"
#include "ui_system/qt_ui/map_control_widget.h"
#include "ui_system/debug_ui/vehicle_analyze_widget.h"

namespace tjs {
	namespace ui {
		QTUIController::QTUIController(Application& application)
			: _application(application) {
		}

		QTUIController::~QTUIController() {
			// Request application to quit
			if (m_app) {
				m_app->quit();
			}
		}

		void QTUIController::run() {
			m_app = std::make_unique<QApplication>(
				_application.commandLine().argc(),
				_application.commandLine().argv());
			createAndShowMainWindow();
		}

		void QTUIController::update() {
			m_app->processEvents();
		}

		void QTUIController::createAndShowMainWindow() {
			// Create main window
			MainWindow* window = new MainWindow(_application);
			window->setWindowTitle("TJS");
			window->resize(400, 800);

			// Create central widget and layout
			QWidget* centralWidget = new QWidget(window);
			QVBoxLayout* layout = new QVBoxLayout(centralWidget);

			RenderMetricsWidget* fpsLabel = new RenderMetricsWidget(_application, window);
			layout->addWidget(fpsLabel);

			MapControlWidget* mapControlWidget = new MapControlWidget(_application, window);
			layout->addWidget(mapControlWidget);

			VehicleAnalyzeWidget* vehicleAnalyzeWidget = new VehicleAnalyzeWidget(_application);
			layout->addWidget(vehicleAnalyzeWidget);

			// Add widgets
			QPushButton* button = new QPushButton("Quit");
			QObject::connect(button, &QPushButton::clicked, [this]() {
				_application.setFinished();
			});
			layout->addWidget(button);

			window->setCentralWidget(centralWidget);
			window->show();
		}
	} // namespace ui
} // namespace tjs
