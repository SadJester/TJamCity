#pragma once

#include <QWidget>
#include <QLabel>
#include <QCheckBox>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::ui {
	class MapAnalyzerWidget : public QWidget {
	public:
		explicit MapAnalyzerWidget(Application& app);

	private slots:
		void updateInfo();
		void onNetworkOnlyChanged(int state);

	private:
		Application& _application;
		QLabel* _nodeId;
		QLabel* _coords;
		QCheckBox* _networkOnly = nullptr;
	};
} // namespace tjs::ui
