#include "stdafx.h"

#include "app_launcher.h"

#include <Application.h>
#include <ui_system/ui_system.h>
#include <render/sdl/sdl_renderer.h>
#include <render/render_constants.h>
#include <visualization/scene_system.h>
#include <visualization/scene_creator.h>

#include <project/project.h>

// Core systems
#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_system.h>

// Store models
#include <core/store_models/vehicle_analyze_data.h>
#include <data/map_renderer_data.h>
#include <data/persistent_render_data.h>
#include <data/simulation_debug_data.h>

// TODO: Place somwhere to be more pretty
#include "visualization/Scene.h"
#include "visualization/scene_system.h"
#include "visualization/elements/map_element.h"
#include "data/persistent_render_data.h"

#include <logic/map/vehicle_targeting.h>
#include <logic/map/lanes_selector.h>
#include <logic/map/map_positioning.h>

namespace tjs {

	void setup_store_models(Application& app) {
		app.stores().create<core::model::VehicleAnalyzeData>();
		app.stores().create<core::model::MapRendererData>();
		app.stores().create<core::model::PersistentRenderData>();
		app.stores().create<core::model::SimulationDebugData>();
	}

	void setup_logic(Application& app) {
		app.logic_modules().create<app::logic::VehicleTargeting>(app);
		app.logic_modules().create<app::logic::LanesSelector>(app);
		app.logic_modules().create<app::logic::MapPositioning>(app);
	}

	int launch(int argc, char* argv[]) {
		tjs::Application application(
			argc,
			argv);

		setup_store_models(application);
		setup_logic(application);

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

		// Open first map that was opened earlier
		open_map_simulation_reinit(application.settings().general.selectedFile, application);

		application.run();

		return 0;
	}
} // namespace tjs
