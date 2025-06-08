#pragma once

#include <render/render_events.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {

	class MapRenderEventsListener : public render::IRenderEventListener {
	public:
		MapRenderEventsListener(Application& app, float maxDistance = 10.0f);
		void on_mouse_event(const render::RendererMouseEvent& event) override;

	private:
		Application& _application;
		float _maxDistance;
	};

} // namespace tjs::visualization
