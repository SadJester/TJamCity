#include "stdafx.h"

#include "app_launcher.h"

#include <Application.h>
#include <ui_system/ui_system.h>
#include <render/sdl/sdl_renderer.h>
#include <render/render_constants.h>
#include <visualization/scene_system.h>
#include <visualization/scene_creator.h>

#include <core/data_layer/world_data.h>
#include <core/data_layer/world_creator.h>

#include <core/simulation/simulation_system.h>

// Store models
#include <core/store_models/vehicle_analyze_data.h>
#include <data/map_renderer_data.h>

namespace tjs {

	void setup_store_models(Application& app) {
		app.stores().add_model<core::model::VehicleAnalyzeData>();
		app.stores().add_model<core::model::MapRendererData>();
	}

	int launch(int argc, char* argv[]) {
		tjs::Application application(
			argc,
			argv);

		setup_store_models(application);

		auto worldData = std::make_unique<tjs::core::WorldData>();
		auto simulationSystem = std::make_unique<core::simulation::TrafficSimulationSystem>(*worldData, application.stores());

		application.setup(
			std::make_unique<tjs::render::SDLRenderer>(application),
			std::make_unique<tjs::UISystem>(application),
			std::make_unique<tjs::visualization::SceneSystem>(application),
			std::move(worldData),
			std::move(simulationSystem));

		application.initialize();

		// TODO: Will move to user settings in some time
		application.renderer().set_clear_color(tjs::render::RenderConstants::BASE_CLEAR_COLOR);
		tjs::visualization::prepareScene(application);

		application.run();

		return 0;
	}
} // namespace tjs
