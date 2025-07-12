#include <stdafx.h>

#include <visualization/elements/path_renderer.h>

#include "render/render_base.h"
#include "visualization/visualization_constants.h"
#include "Application.h"

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>
#include <core/data_layer/road_network.h>
#include <core/math_constants.h>
#include <core/store_models/vehicle_analyze_data.h>
#include <core/simulation/agent/agent_data.h>
#include <data/map_renderer_data.h>

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

		auto& path = model->agent->path;
		std::vector<Position> toVisitPoints;
		toVisitPoints.reserve(path.size() + 2);

		const auto _add_point = [&toVisitPoints, this](const Coordinates& coordinates) {
			toVisitPoints.push_back(convert_to_screen(
				coordinates,
				_mapRendererData.screen_center,
				_mapRendererData.metersPerPixel));
		};

		_add_point(model->agent->vehicle->coordinates);
		if (!path.empty()) {
			_add_point(path[0]->start_node->coordinates);
		}

		// To visit nodes
		for (size_t i = 0; i < path.size(); ++i) {
			_add_point(path[i]->end_node->coordinates);
		}

		// only last segment - position of goal
		if (path.empty()) {
			_add_point(model->agent->currentGoal->coordinates);
		}

		// TODO: thickness of path in settings
		static float thickness = 3.5f;
		drawThickLine(renderer, toVisitPoints, _mapRendererData.metersPerPixel, thickness, Constants::PATH_COLOR);

		renderer.set_draw_color(FColor::Yellow);
		const auto currentGoal = tjs::visualization::convert_to_screen(
			model->agent->currentGoal->coordinates,
			_mapRendererData.screen_center,
			_mapRendererData.metersPerPixel);
		renderer.draw_circle(currentGoal.x, currentGoal.y, 5.0f, true);
	}
} // namespace tjs::visualization
