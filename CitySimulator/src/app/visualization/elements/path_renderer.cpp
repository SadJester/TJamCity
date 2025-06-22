#include <stdafx.h>

#include <visualization/elements/path_renderer.h>

#include "render/render_base.h"
#include "visualization/visualization_constants.h"
#include "Application.h"

#include <core/data_layer/world_data.h>
#include <core/data_layer/data_types.h>
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
		, _mapRendererData(*application.stores().get_model<core::model::MapRendererData>()) {
	}

	void PathRenderer::init() {
		_mapRendererData = *_application.stores().get_model<core::model::MapRendererData>();
	}

	void PathRenderer::update() {
		SceneNode::update();
	}

	void PathRenderer::render(IRenderer& renderer) {
		TJS_TRACY_NAMED("PathRenderer_Render");
		core::model::VehicleAnalyzeData* model = _application.stores().get_model<core::model::VehicleAnalyzeData>();
		if (model->agent == nullptr) {
			return;
		}

		if (model->agent->currentGoal != nullptr) {
			renderer.set_draw_color(Constants::PATH_COLOR);
			auto point = tjs::visualization::convert_to_screen(
				model->agent->currentGoal->coordinates,
				_mapRendererData.projectionCenter,
				_mapRendererData.screen_center,
				_mapRendererData.metersPerPixel);
			renderer.draw_rect(Rectangle(point.x - 2.5f, point.y - 2.5f, 5.0f, 5.0f), true);
		}

		auto& path = model->agent->path;
		auto& visited_path = model->agent->visitedNodes;
		if (path.empty() && visited_path.empty()) {
			return;
		}

		std::vector<Position> toVisitPoints;
		toVisitPoints.reserve(path.size());

		std::vector<Position> visitedPoints;
		visitedPoints.reserve(visited_path.size());

		renderer.set_draw_color(Constants::PATH_MARK_COLOR);
		for (size_t i = 0; i < visited_path.size(); ++i) {
			auto node = visited_path[i];
			auto point = tjs::visualization::convert_to_screen(
				node->coordinates,
				_mapRendererData.projectionCenter,
				_mapRendererData.screen_center,
				_mapRendererData.metersPerPixel);
			visitedPoints.push_back(point);
			renderer.draw_circle(
				point.x,
				point.y,
				i == 0 ? 5.0f : 3.0f);
		}

		// To visit nodes
		bool markFirst = visited_path.size() == 0;
		for (size_t i = 0; i < path.size(); ++i) {
			auto node = path[i];
			auto point = tjs::visualization::convert_to_screen(
				node->coordinates,
				_mapRendererData.projectionCenter,
				_mapRendererData.screen_center,
				_mapRendererData.metersPerPixel);
			toVisitPoints.push_back(point);
			renderer.draw_circle(
				point.x,
				point.y,
				(i == 0 && markFirst) ? 5.0f : 3.0f);
		}

		// TODO: thickness of path in settings
		static float thickness = 11.0f;
		drawThickLine(renderer, visitedPoints, _mapRendererData.metersPerPixel, thickness, FColor { 0.f, 0.f, 1.0f, 1.0f });
		drawThickLine(renderer, toVisitPoints, _mapRendererData.metersPerPixel, thickness, Constants::PATH_COLOR);

		renderer.set_draw_color(FColor { 0.f, 0.f, 1.0f, 1.0f });
		auto currentGoal = tjs::visualization::convert_to_screen(
			model->agent->currentStepGoal,
			_mapRendererData.projectionCenter,
			_mapRendererData.screen_center,
			_mapRendererData.metersPerPixel);
		renderer.draw_circle(currentGoal.x, currentGoal.y, 4.0f);
	}
} // namespace tjs::visualization
