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

#include <visualization/Scene.h>
#include <visualization/scene_system.h>
#include <visualization/elements/map_element.h>


namespace tjs::visualization {
    using namespace tjs::core;

    PathRenderer::PathRenderer(Application& application)
        : SceneNode("PathRenderer")
        , _application(application) {
    }

    void PathRenderer::init() {
        auto scene = _application.sceneSystem().getScene("General");
		if (scene == nullptr) {
			return;
		}

		_mapElement = dynamic_cast<visualization::MapElement*>(scene->getNode("MapElement"));
    }

    void PathRenderer::update() {
        SceneNode::update();
    }

    void PathRenderer::render(IRenderer& renderer) {
        core::model::VehicleAnalyzeData* model = _application.stores().get_model<core::model::VehicleAnalyzeData>();
        if (model->agent == nullptr) {
            return;
        }

        auto& path = model->agent->path;
        if (path.empty()) {
            return;
        }

        std::vector<Position> screenPoints;
		screenPoints.reserve(path.size());
		for (Node* node : path) {
			screenPoints.push_back(_mapElement->convertToScreen(node->coordinates));
		}

        // TODO: thickness of path in settings
        static float thickness = 11.0f;
        drawThickLine(renderer, screenPoints, _mapElement->getZoomLevel(), thickness, Constants::PATH_COLOR);
    }
}