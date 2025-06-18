#include <core/stdafx.h>

#include <core/map_math/contraction_builder.h>

namespace tjs::core::algo {
	void ContractionBuilder::build_graph(core::RoadNetwork& network) {
		// Clear previous data
		network.adjacency_list.clear();

		for (const auto& [way_id, way] : network.ways) {
			// Skip ways that are not suitable for cars
			if (!way->is_car_accessible()) {
				continue;
			}

			const auto& nodes = way->nodes;

			// Connect sequential nodes in the way
			for (size_t i = 0; i < nodes.size() - 1; ++i) {
				Node* current = nodes[i];
				Node* next = nodes[i + 1];

				// Calculate distance between nodes
				double dist = haversine_distance(current->coordinates, next->coordinates);

				// Add edges based on way direction and lanes
				if (way->isOneway) {
					// For one-way roads, only add forward direction
					if (way->lanesForward > 0) {
						network.adjacency_list[current].emplace_back(next, dist);
					}
				} else {
					// For bidirectional roads, add edges based on lane count
					if (way->lanesForward > 0) {
						network.adjacency_list[current].emplace_back(next, dist);
					}
					if (way->lanesBackward > 0) {
						network.adjacency_list[next].emplace_back(current, dist);
					}
				}
				current->tags = current->tags | NodeTags::Way;
				next->tags = current->tags | NodeTags::Way;
			}
		}
	}

} // namespace tjs::core::algo
