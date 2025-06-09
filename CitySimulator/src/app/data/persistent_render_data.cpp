#include "stdafx.h"
#include "data/persistent_render_data.h"
#include "visualization/elements/map_element.h"
#include "Application.h"
#include "data/simulation_debug_data.h"

#include <core/data_layer/world_data.h>
#include <core/map_math/path_finder.h>

namespace tjs::visualization {

	void recalculate_map_data(Application& app) {
		auto* cache = app.stores().get_model<core::model::PersistentRenderData>();
		auto* debug = app.stores().get_model<core::model::SimulationDebugData>();
		auto* render = app.stores().get_model<core::model::MapRendererData>();
		if (!cache || !render || !debug) {
			return;
		}

		uint64_t selectedId = debug->selectedNode ? debug->selectedNode->node->uid : 0;

		cache->nodes.clear();
		cache->ways.clear();
		cache->vehicles.clear();
		debug->selectedNode = nullptr;
		debug->reachableNodes.clear();

		if (app.worldData().segments().empty()) {
			return;
		}

		auto& segment = *app.worldData().segments().front();

		for (auto& [uid, nodePtr] : segment.nodes) {
			NodeRenderInfo info;
			info.node = nodePtr.get();
			info.screenPos = convert_to_screen(
				nodePtr->coordinates,
				render->projectionCenter,
				render->screen_center,
				render->metersPerPixel);
			cache->nodes.emplace(uid, info);
		}

		const auto& nodes_cache = cache->nodes;
		for (auto& [uid, wayPtr] : segment.ways) {
			WayRenderInfo info;
			info.way = wayPtr.get();
			info.screenPoints.reserve(wayPtr->nodes.size());
			for (auto* node : wayPtr->nodes) {
				info.screenPoints.push_back(convert_to_screen(
					node->coordinates,
					render->projectionCenter,
					render->screen_center,
					render->metersPerPixel));

				if (auto it = nodes_cache.find(node->uid); it != nodes_cache.end()) {
					info.nodes.push_back(&it->second);
				}
			}
			cache->ways.emplace(uid, std::move(info));
		}

		for (auto& vehicle : app.worldData().vehicles()) {
			VehicleRenderInfo info;
			info.vehicle = &vehicle;
			info.screenPos = convert_to_screen(
				vehicle.coordinates,
				render->projectionCenter,
				render->screen_center,
				render->metersPerPixel);
			cache->vehicles.push_back(info);
		}

		if (selectedId != 0) {
			if (auto it = cache->nodes.find(selectedId); it != cache->nodes.end()) {
				it->second.selected = true;
				debug->selectedNode = &it->second;
			}
		}

		if (render->networkOnlyForSelected && debug->selectedNode && segment.road_network) {
			auto nodes = core::algo::PathFinder::reachable_nodes(*segment.road_network, debug->selectedNode->node);
			for (auto* n : nodes) {
				debug->reachableNodes.insert(n->uid);
			}
		}
	}

} // namespace tjs::visualization
