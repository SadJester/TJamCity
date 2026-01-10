#pragma once

#include <visual_system/visual_system_definitions.h>
#include <common/system/threaded_system.h>
#include <logic/logic_base.h>

namespace tjs {
	class Application;
	class IRenderer;
} // namespace tjs

namespace tjs::visualization {
	class SceneSystem;

	class VisualSystem : public common::system::threaded_system<
							 VisualSystem, visual_system_commands, visual_sys_lossy_queue, visual_sys_lossless_queue> {
		friend common::system::threaded_system<VisualSystem, visual_system_commands, visual_sys_lossy_queue, visual_sys_lossless_queue>;

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

	private:
		void _initialize_self_impl() override;
		void _initialize_impl() override;
		void _update_impl() override;
		void _release_impl() override;
		void _release_self_impl() override;

		void handle_command(UpdateRenderParamsCommand&& payload);

	private:
		void _setup_logic();
		void _setup_scene();

	private:
		Application& _app;
		std::unique_ptr<IRenderer> _renderer;
		std::unique_ptr<visualization::SceneSystem> _scene_system;

		LogicHandler _logic_modules;
	};

} // namespace tjs::visualization
