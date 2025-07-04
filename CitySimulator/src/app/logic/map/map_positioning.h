#pragma once

#include <render/render_events.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {

	class MapPositioning : public render::IRenderEventListener {
	public:
		explicit MapPositioning(Application& app);
		void on_mouse_event(const render::RendererMouseEvent& event) override;
		void on_mouse_wheel_event(const render::RendererMouseWheelEvent& event) override;
		void on_mouse_motion_event(const render::RendererMouseMotionEvent& event) override;
		void on_key_event(const render::RendererKeyEvent& event) override;

		void update_map_positioning();

	private:
		Application& _application;
		float _maxDistance;
		bool _dragging = false;
	};

} // namespace tjs::visualization
