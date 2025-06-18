#pragma once

#include <core/data_layer/road_network.h>

namespace tjs::core::algo {
	class LaneConnectorBuilder {
	public:
		static void build_lane_connections(core::RoadNetwork& network);
	};
} // namespace tjs::core::algo
