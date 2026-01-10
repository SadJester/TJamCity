#include "stdafx.h"

#include "app_launcher.h"

#include <Application.h>
#include <ui_system/ui_system.h>
#include <render/sdl/sdl_renderer.h>
#include <render/render_constants.h>
#include <visualization/scene_system.h>

#include <project/project.h>

// Core systems
#include <core/data_layer/world_data.h>
#include <core/simulation/simulation_system.h>

#include <visual_system/visual_system.h>

// Store models
#include <core/store_models/vehicle_analyze_data.h>
#include <data/persistent_render_data.h>
#include <data/render_metrics_data.h>

#include <visual_system/data/map_renderer_data.h>

#include <common/system/system_holder_delegate.h>

// TODO: Place somwhere to be more pretty
#include "visualization/Scene.h"
#include "visualization/scene_system.h"
#include "visualization/elements/map_element.h"
#include "data/persistent_render_data.h"

namespace tjs {

	void setup_store_models(Application& app) {
		app.stores().create<core::model::VehicleAnalyzeData>();
		app.stores().create<core::model::MapRendererData>();
		app.stores().create<core::model::MapRendererShared>();
		app.stores().create<core::model::PersistentRenderData>();
		app.stores().create<core::model::RenderMetricsData>();
	}

	class ApplicationDelegate : public common::system::system_holder_delegate {
	public:
		ApplicationDelegate(Application& app)
			: _app(app) {
		}

		void on_initialization_done() noexcept override {
			_app.uiSystem().post_init();

			open_map_simulation_reinit(_app.settings().general.selectedFile, _app);
		}

		void on_release_done() noexcept override {
		}

	private:
		Application& _app;
	};

	int launch(int argc, char* argv[]) {
		tjs::Application application(argc, argv);

		// First load all settings
		application.load_settings();

		setup_store_models(application);

		auto worldData = std::make_unique<tjs::core::WorldData>();
		auto simulationSystem = std::make_unique<core::simulation::TrafficSimulationSystem>(
			*worldData, application.stores(), application.settings().simulationSettings);

		application.setup(
			std::make_unique<tjs::UISystem>(application),
			std::move(worldData),
			std::move(simulationSystem));

		application.initialize();

		application.systems().create<visualization::VisualSystem>(
			application,
			std::make_unique<tjs::render::SDLRenderer>(application),
			std::make_unique<tjs::visualization::SceneSystem>(application));

		ApplicationDelegate delegate(application);
		application.run(delegate);

		return 0;
	}
} // namespace tjs
