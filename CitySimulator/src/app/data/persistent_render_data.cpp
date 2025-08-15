#include "stdafx.h"
#include "data/persistent_render_data.h"
#include "visualization/elements/map_element.h"
#include "Application.h"

#include <core/simulation/simulation_debug.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/path_finder.h>

namespace tjs::visualization {

	void recalculate_map_data(Application& app) {
		auto debug = app.settings().simulationSettings.debug_data;
		auto* render = app.stores().get_entry<core::model::MapRendererData>();
		if (!render) {
			return;
		}

		uint64_t selectedId = debug.selectedNode ? debug.selectedNode->uid : 0;
		debug.reachableNodes.clear();

		if (app.worldData().segments().empty()) {
			return;
		}

		auto& segment = *app.worldData().segments().front();
		if (render->networkOnlyForSelected && debug.selectedNode && segment.road_network) {
			auto nodes = core::algo::PathFinder::reachable_nodes(*segment.road_network, debug.selectedNode);
			for (auto* n : nodes) {
				debug.reachableNodes.insert(n->uid);
			}
		}
	}

} // namespace tjs::visualization
