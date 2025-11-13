#include <stdafx.h>

#include <visual_system/visual_system.h>

#include <Application.h>
#include <render/render_base.h>
#include <render/render_constants.h>
#include <visualization/scene_system.h>

// Map logic
#include <logic/map/vehicle_targeting.h>
#include <logic/map/lanes_selector.h>
#include <logic/map/map_positioning.h>

// Scene
#include <visualization/scene_system.h>
#include <visualization/Scene.h>
#include <visualization/elements/map_element.h>
#include <visualization/elements/vehicle_renderer.h>
#include <visualization/elements/path_renderer.h>


namespace tjs::visualization
{
    VisualSystem::VisualSystem(
            Application& app,
            std::unique_ptr<IRenderer>&& renderer,
			std::unique_ptr<SceneSystem>&& scene_system
        )
            : _app(app)
			, _renderer(std::move(renderer))
			, _scene_system(std::move(scene_system)) {
		}

    VisualSystem::~VisualSystem() {
    }

    void VisualSystem::_initialize_self_impl() {
        std::cout << "[Sys] Self init" << std::endl;

        _renderer->initialize();
        _scene_system->initialize();

        // TODO: Will move to user settings in some time
        _renderer->set_clear_color(tjs::render::RenderConstants::BASE_CLEAR_COLOR);
    }

    void VisualSystem::_initialize_impl() {
        std::cout << "[Sys] Initialize impl" << std::endl;

        _setup_logic();
        _setup_scene();
    }

    void VisualSystem::_update_impl() {
        _renderer->update();
        _scene_system->update();

        // Rendering
        _renderer->begin_frame();
        _scene_system->render(*_renderer);
        _renderer->end_frame();
    }

    void VisualSystem::_release_impl() {
        std::cout << "[Sys] Release impl" << std::endl;
    }

    void VisualSystem::_release_self_impl() {
        std::cout << "[Sys] Release self impl" << std::endl;

        _scene_system.reset();
        _renderer->release();

        _renderer.reset();
    }

    void VisualSystem::_setup_logic() {
        _logic_modules.create<app::logic::VehicleTargeting>(_app);
		_logic_modules.create<app::logic::LanesSelector>(_app);
		_logic_modules.create<app::logic::MapPositioning>(_app);
        _logic_modules.init();
    }

    void VisualSystem::_setup_scene() {
        auto scene = _scene_system->create_scene("General", 0);
		if (scene == nullptr) {
			return;
		}

		scene->addNode(std::make_unique<MapElement>(_app));
		scene->addNode(std::make_unique<VehicleRenderer>(_app));
		scene->addNode(std::make_unique<PathRenderer>(_app));

		scene->initialize();
    }

} // namespace tjs::visualization

