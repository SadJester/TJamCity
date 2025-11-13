#pragma once
#include "render/IRenderable.h"
#include "render/render_base.h"

namespace tjs {
	class IRenderer;
	class Application;
} // namespace tjs

namespace tjs::visualization {
	class Scene;

	class SceneSystem {
	public:
		using ScenePtr = std::unique_ptr<Scene>;
		using Scenes = std::vector<ScenePtr>;

	public:
		SceneSystem(Application& application);
		~SceneSystem();

		Application& application() {
			return _application;
		}

		void initialize();
		void update();
		void render(IRenderer& renderer);

		void sort_scenes();

		const Scenes& getScenes() const {
			return _scenes;
		}
		Scene* create_scene(std::string name, int priority = 0);
		bool remove_scene(std::string_view name);
		Scene* get_scene(std::string_view name);

	private:
		Application& _application;

		Scenes _scenes;
	};
} // namespace tjs::visualization
