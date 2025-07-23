#include <stdafx.h>

#include <visualization/elements/path_renderer.h>

#include "render/render_base.h"
#include "visualization/visualization_constants.h"
#include "Application.h"

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>
#include <core/data_layer/road_network.h>
#include <core/data_layer/vehicle.h>
#include <core/math_constants.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/agent/agent_data.h>
#include <data/map_renderer_data.h>
#include <visualization/elements/map_element.h>

#include <visualization/Scene.h>
#include <visualization/scene_system.h>
#include <visualization/elements/map_element.h>

namespace tjs::visualization {
	using namespace tjs::core;

	PathRenderer::PathRenderer(Application& application)
		: SceneNode("PathRenderer")
		, _application(application)
		, _mapRendererData(*application.stores().get_entry<core::model::MapRendererData>()) {
	}

	void PathRenderer::init() {
		_mapRendererData = *_application.stores().get_entry<core::model::MapRendererData>();
	}

	void PathRenderer::update() {
		SceneNode::update();
	}

	void PathRenderer::render(IRenderer& renderer) {
		TJS_TRACY_NAMED("PathRenderer_Render");
		core::model::VehicleAnalyzeData* model = _application.stores().get_entry<core::model::VehicleAnalyzeData>();
		if (model->agent == nullptr || model->agent->currentGoal == nullptr) {
			return;
		}

		auto* agent = model->agent;
		auto& path = agent->path;
		size_t path_offset = agent->path_offset;
		if (path.empty() || path_offset >= path.size()) {
			return;
		}

		const Coordinates& vehicle_pos = agent->vehicle->coordinates;

		auto convert = [this](const Coordinates& coordinates) {
			return convert_to_screen_f(coordinates, _mapRendererData.screen_center, _mapRendererData.metersPerPixel);
		};

		// Prepare screen coordinates
		std::vector<FPoint> past_points;
		std::vector<FPoint> future_points;

		// Vehicle position
		past_points.push_back(convert(vehicle_pos));

		// From vehicle position back to start of path (reverse order for blue path)
		for (int i = static_cast<int>(path_offset); i >= 0; --i) {
			if (i != path_offset) {
				past_points.push_back(convert(path[i]->end_node->coordinates));
			}
			past_points.push_back(convert(path[i]->start_node->coordinates));
		}

		// From vehicle forward to destination (green)
		future_points.push_back(convert(vehicle_pos));
		for (size_t i = path_offset; i < path.size(); ++i) {
			future_points.push_back(convert(path[i]->end_node->coordinates));
		}

		// Draw past path in Blue
		static constexpr float thickness = 3.5f;
		drawThickLine(renderer, past_points, _mapRendererData.metersPerPixel, thickness, FColor::Blue);

		// Draw future path in Green
		drawThickLine(renderer, future_points, _mapRendererData.metersPerPixel, thickness, FColor::Green);

		// Draw goal marker
		renderer.set_draw_color(FColor::Yellow);
		const auto current_goal_screen = convert_to_screen(
			agent->currentGoal->coordinates,
			_mapRendererData.screen_center,
			_mapRendererData.metersPerPixel);
		renderer.draw_circle(current_goal_screen.x, current_goal_screen.y, 5.0f, true);
	}

} // namespace tjs::visualization
