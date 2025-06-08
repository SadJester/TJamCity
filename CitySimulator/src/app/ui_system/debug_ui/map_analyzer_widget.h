#pragma once

#include <QWidget>
#include <QLabel>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::ui {
	class MapAnalyzerWidget : public QWidget {
	public:
		explicit MapAnalyzerWidget(Application& app);

	private slots:
		void updateInfo();

	private:
		Application& _application;
		QLabel* _nodeId;
		QLabel* _coords;
	};
} // namespace tjs::ui
