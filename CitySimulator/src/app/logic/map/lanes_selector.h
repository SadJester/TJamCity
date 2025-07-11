#pragma once

#include <render/render_events.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {
	class LanesSelector : public render::IRenderEventListener {
	public:
		explicit LanesSelector(Application& app);
		void on_mouse_event(const render::RendererMouseEvent& event) override;

	private:
		Application& _application;
		float _maxDistance;
	};
} // namespace tjs::visualization
