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

			layout->addWidget(fpsLabel);

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
					.arg(stats.smoothedFPS(), 2, 'f', 2)
					.arg(std::chrono::duration_cast<std::chrono::milliseconds>(stats.frameTime()).count()));

			if (stats.currentFPS() < 30.f) {
				fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
			} else if (stats.currentFPS() < 50.0f) {
				fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: orange;");
			} else {
				fpsLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: green;");
			}

			update();
		}

	} // namespace ui
} // namespace tjs
