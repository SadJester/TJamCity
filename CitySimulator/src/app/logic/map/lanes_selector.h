#pragma once

#include <render/render_events.h>
#include <logic/logic_base.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::app::logic {
	class LanesSelector : public ILogicModule, public render::IRenderEventListener {
	public:
		static std::type_index get_type() {
			return typeid(LanesSelector);
		}

	public:
		explicit LanesSelector(Application& app);
		void on_mouse_event(const render::RendererMouseEvent& event) override;

		void init() override;
		void release() override;

	private:
		float _maxDistance;
	};
} // namespace tjs::app::logic
