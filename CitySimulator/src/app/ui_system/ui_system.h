#pragma once

#include <visual_system/data/map_renderer_data.h>

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
		void update();

		core::model::MapRendererShared::connection& get_render_data_connection() {
			return _render_data_connection;
		}

	private:
		std::unique_ptr<IUIController> _controller;
		Application& _application;

		core::model::MapRendererShared::connection _render_data_connection;
	};
} // namespace tjs
