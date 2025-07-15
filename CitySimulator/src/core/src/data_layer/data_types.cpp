#include <core/stdafx.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/road_network.h>

#include <common/math/bounding_box.h>

namespace tjs::core {

	void WorldSegment::rebuild_grid() {
		double min_x = std::numeric_limits<double>::max() - 1;
		double max_x = -min_x;
		double min_y = std::numeric_limits<double>::max() - 1;
		double max_y = -min_y;
		for (auto& node : nodes) {
			min_x = std::min(node.second->coordinates.x, min_x);
			max_x = std::max(node.second->coordinates.x, max_x);

			min_y = std::min(node.second->coordinates.y, min_y);
			max_y = std::max(node.second->coordinates.y, max_y);
		}

		double diff = std::max(
			max_x - min_x,
			max_y - min_y);

		// naive approach of spatial grid to make 100x100
		spatialGrid.cellSize = diff / 5;

		for (const auto& [_, way] : ways) {
			add_way(spatialGrid, way.get());
		}
	}

	void add_way(SpatialGrid& grid, WayInfo* way) {
		if (way == nullptr) {
			return;
		}

		for (auto* node : way->nodes) {
			grid.add_entry(way, node->coordinates);
		}

		for (auto& edgeHandler : way->edges) {
			const auto& edge = *edgeHandler;
			for (const auto& lane : edge.lanes) {
				if (lane.centerLine.empty()) {
					continue;
				}
				common::BoundingBox box { lane.centerLine.front().x,
					lane.centerLine.front().y,
					lane.centerLine.front().x,
					lane.centerLine.front().y };
				for (const auto& c : lane.centerLine) {
					box.min_x = std::min(box.min_x, c.x);
					box.min_y = std::min(box.min_y, c.y);
					box.max_x = std::max(box.max_x, c.x);
					box.max_y = std::max(box.max_y, c.y);
				}
				grid.add_tree_entry(box, const_cast<Lane*>(&lane));
			}
		}
	}

} // namespace tjs::core
