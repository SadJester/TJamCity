#pragma once

#include <render/render_events.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {

	class MapRenderEventsListener : public render::IRenderEventListener {
	public:
		explicit MapRenderEventsListener(Application& app);
		void on_mouse_event(const render::RendererMouseEvent& event) override;
		void on_mouse_wheel_event(const render::RendererMouseWheelEvent& event) override;
		void on_key_event(const render::RendererKeyEvent& event) override;

	private:
		Application& _application;
		float _maxDistance;

		static double get_changed_step(double metersPerPixel);
	};

} // namespace tjs::visualization
