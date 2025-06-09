#pragma once

#include <QWidget>
#include <QListWidget>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::ui {
	class StrategicAnalyzerWidget : public QWidget {
	public:
		explicit StrategicAnalyzerWidget(Application& app);

	private slots:
		void updateInfo();

	private:
		Application& _application;
		QListWidget* _list = nullptr;
	};
} // namespace tjs::ui
