#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>

#include <events/map_events.h>
#include <events/project_events.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::ui {
	class EdgeInformationWidget : public QWidget {
		//Q_OBJECT

	public:
		explicit EdgeInformationWidget(Application& app);
		~EdgeInformationWidget();
	private slots:
		void handleItemClicked(QTreeWidgetItem* item, int column);

	private:
		void handle_lane_selected(const events::LaneIsSelected& event);
		void handle_open_map(const events::OpenMapEvent& event);

	private:
		void populateTree();
		Application& _application;
		QTreeWidget* _tree = nullptr;
		QLabel* _info = nullptr;
		QTreeWidgetItem* _rootItem = nullptr;
	};
} // namespace tjs::ui
