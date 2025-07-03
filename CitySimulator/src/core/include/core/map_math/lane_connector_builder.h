#pragma once

#include <core/data_layer/road_network.h>

namespace tjs::core::algo {
	class LaneConnectorBuilder {
	public:
		static void build_lane_connections(RoadNetwork& network);
	};

	namespace details {
		struct AdjacentEdges {
			std::vector<Edge*> incoming;
			std::vector<Edge*> outgoing;
			Edge* primary = nullptr;
		};

		AdjacentEdges get_adjacent_edges(RoadNetwork& network, Node* node);
		void process_node(RoadNetwork& network, Node* node);
	} // namespace details
} // namespace tjs::core::algo
