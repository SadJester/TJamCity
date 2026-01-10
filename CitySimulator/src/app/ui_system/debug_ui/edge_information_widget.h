#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>

#include <ui_system/ui_definitions.h>

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
		void handle(const visualization::visual_sys_lossy_queue::message_t& msg);

		void handle_open_map(const events::OpenMapEvent& event);

	private:
		void populateTree();

	private:
		Application& _application;
		ui::ui_event_bus::handler_t _ui_subs_handler {};

		QTreeWidget* _tree = nullptr;
		QLabel* _info = nullptr;
		QTreeWidgetItem* _rootItem = nullptr;
	};
} // namespace tjs::ui
