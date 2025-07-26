#pragma once

#include <core/map_math/earth_math.h>

namespace tjs::core {
	struct RoadNetwork;
	struct Edge_Contract;
} // namespace tjs::core

namespace tjs::core::algo {

	class ContractionBuilder {
	public:
		void build_graph(core::RoadNetwork& network);
	};

	Edge create_edge(Node* start_node,
		Node* end_node,
		WayInfo* way,
		double dist,
		core::LaneOrientation orientation);

} // namespace tjs::core::algo
