#pragma once
#include <core/enum_flags.h>
#include <core/simulation_constants.h>

#include <core/data_layer/way_info.h>
#include <core/data_layer/node.h>
#include <core/data_layer/road_network.h>
#include <common/spatial/spatial_grid.h>

namespace tjs::core {
	using SpatialGrid = tjs::common::SpatialGrid<WayInfo, Lane>;

	struct SegmentBoundingBox {
		Coordinates left;
		Coordinates right;
		Coordinates top;
		Coordinates bottom;
	};

	struct WorldSegment {
		SegmentBoundingBox boundingBox;
		std::unordered_map<uint64_t, std::unique_ptr<Node>> nodes;
		std::unordered_map<uint64_t, std::unique_ptr<WayInfo>> ways;
		std::unordered_map<uint64_t, std::unique_ptr<Junction>> junctions;

		// Sorted by layer
		std::vector<WayInfo*> sorted_ways;

		std::unique_ptr<RoadNetwork> road_network;
		SpatialGrid spatialGrid;

		static std::unique_ptr<WorldSegment> create() {
			auto segment = std::make_unique<WorldSegment>();
			segment->road_network = std::make_unique<RoadNetwork>();
			return segment;
		}

		void rebuild_grid();
	};

	void add_way(SpatialGrid& grid, WayInfo* way);
} // namespace tjs::core
