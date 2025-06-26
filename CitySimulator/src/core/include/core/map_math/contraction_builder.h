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

} // namespace tjs::core::algo
