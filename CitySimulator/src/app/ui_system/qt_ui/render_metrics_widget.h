#pragma once

#include <QWidget>
#include <QLabel>

namespace tjs {
	class Application;

	namespace ui {
		class MainWindow;

		class RenderMetricsWidget : public QWidget {
			//Q_OBJECT

		private:
			QLabel* fpsLabel;
			QLabel* simulationUpdateLabel;
			QLabel* systemsUpdateLabel;
			QLabel* renderTimeLabel;
			QLabel* trianglesLabel;
			Application& _app;

		public:
			RenderMetricsWidget(Application& app, MainWindow* parent);

		private slots:
			void updateFrame();
		};
	} // namespace ui
} // namespace tjs
