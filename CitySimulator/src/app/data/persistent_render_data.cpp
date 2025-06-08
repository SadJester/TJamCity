#include "stdafx.h"
#include "data/persistent_render_data.h"
#include "visualization/elements/map_element.h"
#include "Application.h"

#include <core/data_layer/world_data.h>

namespace tjs::visualization {

	void recalculate_map_data(Application& app) {
		auto* cache = app.stores().get_model<core::model::PersistentRenderData>();
		auto* render = app.stores().get_model<core::model::MapRendererData>();
		if (!cache || !render) {
			return;
		}

		cache->nodes.clear();
		cache->ways.clear();
		cache->vehicles.clear();
		cache->selectedNode = nullptr;

		if (app.worldData().segments().empty()) {
			return;
		}

		auto& segment = *app.worldData().segments().front();

		for (auto& [uid, nodePtr] : segment.nodes) {
			core::model::NodeRenderInfo info;
			info.node = nodePtr.get();
			info.screenPos = convert_to_screen(
				nodePtr->coordinates,
				render->projectionCenter,
				render->screen_center,
				render->metersPerPixel);
			cache->nodes.emplace(uid, info);
		}

		for (auto& [uid, wayPtr] : segment.ways) {
			core::model::WayRenderInfo info;
			info.way = wayPtr.get();
			info.screenPoints.reserve(wayPtr->nodes.size());
			for (auto* node : wayPtr->nodes) {
				info.screenPoints.push_back(convert_to_screen(
					node->coordinates,
					render->projectionCenter,
					render->screen_center,
					render->metersPerPixel));
			}
			cache->ways.emplace(uid, std::move(info));
		}

		for (auto& vehicle : app.worldData().vehicles()) {
			core::model::VehicleRenderInfo info;
			info.vehicle = &vehicle;
			info.screenPos = convert_to_screen(
				vehicle.coordinates,
				render->projectionCenter,
				render->screen_center,
				render->metersPerPixel);
			cache->vehicles.push_back(info);
		}
	}

} // namespace tjs::visualization
