#pragma once

#include <visual_system/visual_system_definitions.h>
#include <logic/logic_base.h>

#include <common/system/threaded_system.h>
#include <common/sync/coordinated_gate.h>

namespace tjs {
	class Application;
	class IRenderer;
} // namespace tjs

namespace tjs::visualization {
	class SceneSystem;

	using v_threaded_system = common::system::threaded_system<
		VisualSystem,
		visual_system_commands,
		visual_sys_lossy_queue,
		visual_sys_lossless_queue
		// event_bus
		>;

	class VisualSystem : public v_threaded_system {
		friend v_threaded_system;

	public:
		using self_type = VisualSystem;

	public:
		VisualSystem(
			Application& app,
			std::unique_ptr<IRenderer>&& renderer,
			std::unique_ptr<SceneSystem>&& scene_system);
		~VisualSystem();

		IRenderer& renderer() {
			return *_renderer;
		}

		visualization::SceneSystem& scene_system() {
			return *_scene_system;
		}

		visualization::visual_event_bus& event_bus() {
			return _event_bus;
		}

		// Overrides

	private:
		void _initialize_self_impl() override;
		void _initialize_impl() override;
		void _update_impl() override;
		void _release_impl() override;
		void _release_self_impl() override;

		// Commands

	private:
		void handle_command(UpdateRenderParamsCommand&& payload);

		// Internal logic

	private:
		void _setup_logic();
		void _setup_scene();

	private:
		Application& _app;
		common::sync::coordinated_gate _gate;

		std::unique_ptr<IRenderer> _renderer;
		std::unique_ptr<visualization::SceneSystem> _scene_system;

		LogicHandler _logic_modules;

		visualization::visual_event_bus _event_bus;
	};

} // namespace tjs::visualization
