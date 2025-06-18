#include <core/stdafx.h>

#include <core/map_math/contraction_builder.h>

namespace tjs::core::algo {
	void ContractionBuilder::build_graph(core::RoadNetwork& network) {
		// Clear previous data
		network.adjacency_list.clear();
		network.edges.clear();
		network.edge_graph.clear();

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
					if (way->lanesForward > 0) {
						network.adjacency_list[current].emplace_back(next, dist);
					}
				} else {
					if (way->lanesForward > 0) {
						network.adjacency_list[current].emplace_back(next, dist);
					}
					if (way->lanesBackward > 0) {
						network.adjacency_list[next].emplace_back(current, dist);
					}
				}

				if (way->lanesForward > 0) {
					network.edges.emplace_back();
					auto& edge = network.edges.back();
					edge.start_node = current;
					edge.end_node = next;
					edge.way = way;
					edge.orientation = core::LaneOrientation::Forward;
					edge.length = dist;
					edge.lanes.resize(static_cast<size_t>(way->lanesForward));
					for (size_t l = 0; l < edge.lanes.size(); ++l) {
						auto& lane = edge.lanes[l];
						lane.parent = &edge;
						lane.orientation = core::LaneOrientation::Forward;
						lane.width = way->laneWidth;
						lane.centerLine = { current->coordinates, next->coordinates };
						if (l < way->forwardTurns.size()) {
							lane.turn = way->forwardTurns[l];
						} else {
							lane.turn = core::TurnDirection::Straight;
						}
					}
					network.edge_graph[current].push_back(&edge);
				}

				if (!way->isOneway && way->lanesBackward > 0) {
					network.edges.emplace_back();
					auto& edge = network.edges.back();
					edge.start_node = next;
					edge.end_node = current;
					edge.way = way;
					edge.orientation = core::LaneOrientation::Backward;
					edge.length = dist;
					edge.lanes.resize(static_cast<size_t>(way->lanesBackward));
					for (size_t l = 0; l < edge.lanes.size(); ++l) {
						auto& lane = edge.lanes[l];
						lane.parent = &edge;
						lane.orientation = core::LaneOrientation::Backward;
						lane.width = way->laneWidth;
						lane.centerLine = { next->coordinates, current->coordinates };
						if (l < way->backwardTurns.size()) {
							lane.turn = way->backwardTurns[l];
						} else {
							lane.turn = core::TurnDirection::Straight;
						}
					}
					network.edge_graph[next].push_back(&edge);
				}

				current->tags = current->tags | NodeTags::Way;
				next->tags = current->tags | NodeTags::Way;
			}
		}
	}

} // namespace tjs::core::algo
