#pragma once

#include <render/render_events.h>
#include <logic/logic_base.h>

namespace tjs {
	class Application;
} // namespace tjs

namespace tjs::app::logic {
	class VehicleTargeting : public ILogicModule, public render::IRenderEventListener {
	public:
		static std::type_index get_type() {
			return typeid(VehicleTargeting);
		}

		explicit VehicleTargeting(Application& app);

		void init() override;
		void release() override;

		void on_mouse_event(const render::RendererMouseEvent& event) override;

	private:
		float _maxDistance;
	};
} // namespace tjs::app::logic
