#include "ui_system/qt_ui/render_metrics_widget.h"
#include "ui_system/qt_ui/main_window.h"

#include "Application.h"

#include <QTimer>
#include <QWidget>
#include <QVBoxLayout>

namespace tjs {
	namespace ui {
		RenderMetricsWidget::RenderMetricsWidget(Application& app, MainWindow* parent)
			: QWidget() // Don't set parent in constructor
			, _app(app) {
			// Create FPS label directly on this widget
			QVBoxLayout* layout = new QVBoxLayout(this);
			layout->setContentsMargins(0, 0, 0, 0);

			// Create FPS label
			fpsLabel = new QLabel("FPS: 00 | Frame time: 00 ms", this);
			fpsLabel->setAlignment(Qt::AlignLeft);
			fpsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

			// Create simulation update label
			simulationUpdateLabel = new QLabel("Simulation update: 0 ms", this);
			simulationUpdateLabel->setAlignment(Qt::AlignLeft);
			simulationUpdateLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			simulationUpdateLabel->setStyleSheet("font-size: 12px; font-weight: bold;");

			// Create systems update label
			systemsUpdateLabel = new QLabel("Systems update: 0 ms", this);
			systemsUpdateLabel->setAlignment(Qt::AlignLeft);
			systemsUpdateLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			systemsUpdateLabel->setStyleSheet("font-size: 12px; font-weight: bold;");

			// Create render time label
			renderTimeLabel = new QLabel("Render time: 0 ms", this);
			renderTimeLabel->setAlignment(Qt::AlignLeft);
			renderTimeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			renderTimeLabel->setStyleSheet("font-size: 12px; font-weight: bold;");

			layout->addWidget(fpsLabel);
			layout->addWidget(simulationUpdateLabel);
			layout->addWidget(systemsUpdateLabel);
			layout->addWidget(renderTimeLabel);

			// Connect to parent's timer
			if (parent && parent->timer()) {
				connect(parent->timer(), &QTimer::timeout, this, &RenderMetricsWidget::updateFrame);
			}
		}

		void RenderMetricsWidget::updateFrame() {
			// Update FPS label
			auto& stats = _app.frameStats();
			fpsLabel->setText(
				QString("FPS: %1 (%2 ms)")
					.arg(stats.fps().get(), 2, 'f', 2)
					.arg(std::chrono::duration_cast<std::chrono::milliseconds>(stats.frame_time()).count()));

			if (stats.fps().get_raw() < 30.f) {
				fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
			} else if (stats.fps().get_raw() < 50.0f) {
				fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: orange;");
			} else {
				fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: green;");
			}

			// Update simulation update time
			simulationUpdateLabel->setText(
				QString("Simulation update: %1 ms")
					.arg(stats.simulation_update().get() * 1000.0f, 0, 'f', 2));

			// Update systems update time
			systemsUpdateLabel->setText(
				QString("Systems update: %1 ms")
					.arg(stats.systems_update().get() * 1000.0f, 0, 'f', 2));

			// Update render time
			renderTimeLabel->setText(
				QString("Render time: %1 ms")
					.arg(stats.render_time().get() * 1000.0f, 0, 'f', 2));

			update();
		}

	} // namespace ui
} // namespace tjs
