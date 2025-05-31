#include <stdafx.h>

#include <visualization/scene_creator.h>

#include <visualization/scene_system.h>
#include <visualization/Scene.h>
#include <visualization/elements/map_element.h>
#include <visualization/elements/vehicle_renderer.h>
#include <visualization/elements/path_renderer.h>

#include <Application.h>

namespace tjs::visualization {
	void prepareScene(tjs::Application& app) {
		auto& sceneSystem = app.sceneSystem();

		auto scene = sceneSystem.createScene("General", 0);
		if (scene == nullptr) {
			return;
		}

		scene->addNode(std::make_unique<MapElement>(app));
		scene->addNode(std::make_unique<VehicleRenderer>(app));
		scene->addNode(std::make_unique<PathRenderer>(app));

		scene->initialize();
	}
} // namespace tjs::visualization
