#include <core/stdafx.h>

#include <core/dataLayer/data_types.h>


namespace tjs::core {
    WorldSegment::~WorldSegment() {   
    }


    void SpatialGrid::add_way(WayInfo* way) {
        for (auto& node : way->nodes) {
            auto gridKey = std::make_pair(
                static_cast<int>(way->nodes[0]->coordinates.latitude / cellSize),
                static_cast<int>(way->nodes[0]->coordinates.longitude / cellSize)
            );
            auto& cell = spatialGrid[gridKey];
            cell.push_back(way);   
        }
    }

    void WorldSegment::rebuild_grid() {
        for (auto& way : ways) {
            spatialGrid.add_way(way.second.get());
        }
    }

}
