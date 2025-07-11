#pragma once

#include <render/render_events.h>
#include <logic/logic_base.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::visualization {
	class LanesSelector : public ILogicModule, public render::IRenderEventListener {
	public:
		explicit LanesSelector(Application& app);
		void on_mouse_event(const render::RendererMouseEvent& event) override;

	private:
		float _maxDistance;
	};
} // namespace tjs::visualization
