#include <core/stdafx.h>

#include <core/data_layer/data_types.h>

namespace tjs::core {
	void SpatialGrid::add_way(WayInfo* way) {
		if (way == nullptr) {
			return;
		}

		for (auto& node : way->nodes) {
			auto gridKey = std::make_pair(
				static_cast<int>(node->coordinates.x / cellSize),
				static_cast<int>(node->coordinates.y / cellSize));
			auto& cell = spatialGrid[gridKey];
			if (std::ranges::find(cell, way) == cell.end()) {
				cell.emplace_back(way);
			}
		}
	}

	std::optional<std::reference_wrapper<const SpatialGrid::WaysInCell>> SpatialGrid::get_ways_in_cell(Coordinates coordinates) const {
		return get_ways_in_cell(
			static_cast<int>(coordinates.x / cellSize),
			static_cast<int>(coordinates.y / cellSize));
	}

	std::optional<std::reference_wrapper<const SpatialGrid::WaysInCell>> SpatialGrid::get_ways_in_cell(int x, int y) const {
		auto it = spatialGrid.find(std::make_pair(x, y));
		if (it != spatialGrid.end()) {
			return it->second;
		}
		return std::nullopt;
	}

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
			spatialGrid.add_way(way.get());
		}
	}

} // namespace tjs::core
