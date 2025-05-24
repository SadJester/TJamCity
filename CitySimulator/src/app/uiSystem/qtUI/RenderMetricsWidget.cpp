#include "uiSystem/qtUI/RenderMetricsWidget.h"
#include "uiSystem/qtUI/MainWindow.h"

#include "Application.h"

#include <QTimer>
#include <QWidget>
#include <QVBoxLayout>

namespace tjs {
	namespace ui {
		RenderMetricsWidget::RenderMetricsWidget(Application& app, MainWindow* parent)
			: QWidget(static_cast<QWidget*>(parent))
			, _app(app) {
			// Create a central widget and layout
			QWidget* centralWidget = new QWidget(this);
			QVBoxLayout* layout = new QVBoxLayout(centralWidget);

			// Create FPS label
			fpsLabel = new QLabel("FPS: 00 | Frame time: 00 ms", this);
			fpsLabel->setAlignment(Qt::AlignCenter);
			fpsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold;");

			layout->addWidget(fpsLabel);
			connect(parent->timer(), &QTimer::timeout, this, &RenderMetricsWidget::updateFrame);
		}

		void RenderMetricsWidget::updateFrame() {
			// Update FPS label
			auto& stats = _app.frameStats();
			fpsLabel->setText(
				QString("FPS: %1 (%2 ms)")
					.arg(stats.smoothedFPS(), 2, 'f', 2)
					.arg(std::chrono::duration_cast<std::chrono::milliseconds>(stats.frameTime()).count()));

			if (stats.currentFPS() < 30.f) {
				fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: red;");
			} else if (stats.currentFPS() < 50.0f) {
				fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: orange;");
			} else {
				fpsLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: green;");
			}

			update();
		}

	} // namespace ui
} // namespace tjs
