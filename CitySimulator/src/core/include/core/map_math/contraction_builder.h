#pragma once

#include <core/map_math/earth_math.h>

namespace tjs::core {
	struct RoadNetwork;
	struct Edge;
} // namespace tjs::core

namespace tjs::core::algo {

	class ContractionBuilder {
	public:
		void build_contraction_hierarchy(core::RoadNetwork& network);

	private:
		void compute_node_priorities(core::RoadNetwork& network);
		double compute_edge_difference(core::RoadNetwork& network, uint64_t node_id);
		void contract_node(core::RoadNetwork& network, uint64_t node_id);
		bool should_add_shortcut(core::RoadNetwork& network,
			const core::Edge& in_edge,
			const core::Edge& out_edge);
		void add_shortcut(core::RoadNetwork& network,
			const core::Edge& in_edge,
			const core::Edge& out_edge,
			uint64_t contracted_node);
		bool has_witness_path(core::RoadNetwork& network,
			uint64_t from,
			uint64_t to,
			double limit);
		uint64_t get_next_node();
		int calculate_required_shortcuts(RoadNetwork& network, uint64_t node_id);

	private:
		std::priority_queue<std::pair<double, uint64_t>> priority_queue;
	};

} // namespace tjs::core::algo
