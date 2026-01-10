#pragma once

#include <visual_system/visual_system_definitions.h>

#include <render/render_events.h>
#include <logic/logic_base.h>

namespace tjs {
	class Application;
	namespace core::model {
		struct MapRendererShared;
	} // namespace core::model
} // namespace tjs

namespace tjs::app::logic {

	// TODO{threaded}: Move to totally in render thread. Not in Application
	class MapPositioning : public ILogicModule, public render::IRenderEventListener {
	public:
		static std::type_index get_type() {
			return typeid(MapPositioning);
		}

	public:
		explicit MapPositioning(Application& app);

		void init() override;
		void release() override;

		void on_mouse_event(const render::RendererMouseEvent& event) override;
		void on_mouse_wheel_event(const render::RendererMouseWheelEvent& event) override;
		void on_mouse_motion_event(const render::RendererMouseMotionEvent& event) override;
		void on_key_event(const render::RendererKeyEvent& event) override;

		void update_map_positioning();

		void handle(const events::OpenMapEvent&);

	private:
		float _maxDistance;
		bool _dragging = false;

		core::model::MapRendererShared& _render_data;
		visualization::visual_event_bus::handler_t _subs_handler {};
	};

} // namespace tjs::app::logic
