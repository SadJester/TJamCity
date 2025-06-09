#include "stdafx.h"

#include "ui_system/qt_ui/qt_controller.h"
#include "ui_system/qt_ui/main_window.h"
#include "Application.h"

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>

/// TODO: Place somwhere to be more pretty
#include "ui_system/qt_ui/render_metrics_widget.h"
#include "ui_system/qt_ui/map_control_widget.h"
#include "ui_system/qt_ui/time_control_widget.h"
#include "ui_system/debug_ui/vehicle_analyze_widget.h"
#include "ui_system/debug_ui/map_analyzer_widget.h"
#include "ui_system/debug_ui/strategic_analyzer_widget.h"

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
			window->resize(700, 800);

			// Create main widget to hold everything
			QWidget* mainWidget = new QWidget(window);
			QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
			mainLayout->setSpacing(0);
			mainLayout->setContentsMargins(0, 0, 0, 0);

			// Top widgets
			RenderMetricsWidget* fpsLabel = new RenderMetricsWidget(_application, window);
			mainLayout->addWidget(fpsLabel);

			TimeControlWidget* timeControlWidget = new TimeControlWidget(_application, mainWidget);
			mainLayout->addWidget(timeControlWidget);

			// Create scroll area for the rest
			QScrollArea* scrollArea = new QScrollArea(mainWidget);
			scrollArea->setWidgetResizable(true);
			scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
			scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

			QWidget* scrollContent = new QWidget(scrollArea);
			QHBoxLayout* columnsLayout = new QHBoxLayout(scrollContent);
			QVBoxLayout* mapColumn = new QVBoxLayout();
			QVBoxLayout* debugColumn = new QVBoxLayout();
			columnsLayout->addLayout(mapColumn);
			columnsLayout->addLayout(debugColumn);

			MapControlWidget* mapControlWidget = new MapControlWidget(_application, scrollContent);
			mapColumn->addWidget(mapControlWidget);

			MapAnalyzerWidget* analyzerWidget = new MapAnalyzerWidget(_application);
			analyzerWidget->setParent(scrollContent);
			debugColumn->addWidget(analyzerWidget);

			VehicleAnalyzeWidget* vehicleAnalyzeWidget = new VehicleAnalyzeWidget(_application);
			vehicleAnalyzeWidget->setParent(scrollContent);
			debugColumn->addWidget(vehicleAnalyzeWidget);

			StrategicAnalyzerWidget* strategicWidget = new StrategicAnalyzerWidget(_application);
			strategicWidget->setParent(scrollContent);
			debugColumn->addWidget(strategicWidget);

			mapControlWidget->setVehicles(vehicleAnalyzeWidget);

			scrollContent->setLayout(columnsLayout);
			scrollArea->setWidget(scrollContent);

			// Add scroll area to main layout
			mainLayout->addWidget(scrollArea);

			// Create a container for the Quit button with a line above it
			QWidget* quitContainer = new QWidget(mainWidget);
			QVBoxLayout* quitLayout = new QVBoxLayout(quitContainer);
			quitLayout->setSpacing(0);
			quitLayout->setContentsMargins(10, 0, 10, 10);

			// Add a horizontal line
			QFrame* line = new QFrame(quitContainer);
			line->setFrameShape(QFrame::HLine);
			line->setFrameShadow(QFrame::Sunken);
			quitLayout->addWidget(line);

			// Add Quit button
			QPushButton* button = new QPushButton("Quit", quitContainer);
			QObject::connect(button, &QPushButton::clicked, [this]() {
				_application.setFinished();
			});
			quitLayout->addWidget(button);

			// Add quit container to main layout
			mainLayout->addWidget(quitContainer);

			window->setCentralWidget(mainWidget);
			window->show();
		}
	} // namespace ui
} // namespace tjs
