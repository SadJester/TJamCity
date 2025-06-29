#pragma once

#include <core/data_layer/road_network.h>

namespace tjs::core::algo {
	class LaneConnectorBuilder {
	public:
		static void build_lane_connections(RoadNetwork& network);
	};

	namespace details {
		void process_node(RoadNetwork& network, Node* node);
	}
} // namespace tjs::core::algo
