#include <core/stdafx.h>

#include <core/dataLayer/data_types.h>


namespace tjs::core {
    void SpatialGrid::add_way(WayInfo* way) {
        if (way == nullptr) {
            return;
        }

        for (auto& node : way->nodes) {
            auto gridKey = std::make_pair(
                static_cast<int>(node->coordinates.latitude / cellSize),
                static_cast<int>(node->coordinates.longitude / cellSize)
            );
            auto& cell = spatialGrid[gridKey];
            if (std::ranges::find(cell, way) == cell.end()) {
                cell.emplace_back(way);
            }
        }
    }

    std::optional<std::reference_wrapper<const SpatialGrid::WaysInCell>> SpatialGrid::get_ways_in_cell(Coordinates coordinates) const {
        return get_ways_in_cell(
            static_cast<int>(coordinates.latitude / cellSize),
            static_cast<int>(coordinates.longitude / cellSize)
        );
    }

    std::optional<std::reference_wrapper<const SpatialGrid::WaysInCell>> SpatialGrid::get_ways_in_cell(int x, int y) const {
        auto it = spatialGrid.find(std::make_pair(x, y));
        if (it != spatialGrid.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void WorldSegment::rebuild_grid() {
        for (const auto& [_, way] : ways) {
            spatialGrid.add_way(way.get());
        }
    }

}
