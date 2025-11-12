#pragma once

namespace tjs {
	class Application;
}

namespace tjs::visualization {
	class SceneSystem;

	void prepareScene(SceneSystem& sceneSystem, Application& app);
} // namespace tjs::visualization
