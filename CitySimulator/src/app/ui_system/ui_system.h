#pragma once

#include <visual_system/data/map_renderer_data.h>
#include <visual_system/visual_system_definitions.h>

#include <ui_system/ui_definitions.h>

namespace tjs {
	class Application;

	class IUIController {
	public:
		virtual ~IUIController() {}
		virtual void run() = 0;
		virtual void update() = 0;
	};

	class UISystem {
	public:
		UISystem(Application& application)
			: _application(application) {
		}

		void initialize();

		// threaded_system::_initialize_impl - after all system initialization
		void post_init();
		void update();

		core::model::MapRendererShared::connection& get_render_data_connection() {
			return _render_data_connection;
		}

		ui::ui_event_bus& visual_event_bus() {
			return _visual_events_bus;
		}

	private:
		void _process_events();

	private:
		std::unique_ptr<IUIController> _controller;
		Application& _application;

		core::model::MapRendererShared::connection _render_data_connection;

		visualization::visual_sys_lossy_queue::reader _visual_lossy_reader;
		visualization::visual_sys_lossless_queue::reader _visual_lossless_reader;

		ui::ui_event_bus _visual_events_bus;
	};
} // namespace tjs
