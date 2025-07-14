#pragma once

#include <QWidget>
#include <QLabel>
#include <QLocale>

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

		private:
			QLocale _locale { QLocale::system() };
			int _update_counter = 0;
		};
	} // namespace ui
} // namespace tjs
