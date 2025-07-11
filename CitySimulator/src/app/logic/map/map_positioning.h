#pragma once

#include <render/render_events.h>
#include <logic/logic_base.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::app::logic {

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

	private:
		float _maxDistance;
		bool _dragging = false;
	};

} // namespace tjs::app::logic
