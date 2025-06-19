#include <core/stdafx.h>

#include <core/map_math/contraction_builder.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::algo {
	void ContractionBuilder::build_graph(core::RoadNetwork& network) {
		// Clear previous data
		network.adjacency_list.clear();
		network.edges.clear();
		network.edge_graph.clear();

		std::unordered_map<Node*, std::vector<size_t>> edge_graph_indices;

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

					double heading = bearing(current->coordinates, next->coordinates);
					size_t lane_count = edge.lanes.size();
					for (size_t l = 0; l < lane_count; ++l) {
						auto& lane = edge.lanes[l];
						lane.parent = &edge;
						lane.orientation = core::LaneOrientation::Forward;
						lane.width = way->laneWidth;
						double offset = (static_cast<double>(l) - (static_cast<double>(lane_count - 1) / 2.0)) * lane.width;
						Coordinates start = offset_coordinate(current->coordinates, heading, offset);
						Coordinates end = offset_coordinate(next->coordinates, heading, offset);
						lane.centerLine = { start, end };
						if (l < way->forwardTurns.size()) {
							lane.turn = way->forwardTurns[l];
						} else {
							lane.turn = core::TurnDirection::Straight;
						}
					}
					edge_graph_indices[current].push_back(network.edges.size() - 1);
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

					double heading = bearing(next->coordinates, current->coordinates);
					size_t lane_count = edge.lanes.size();
					for (size_t l = 0; l < lane_count; ++l) {
						auto& lane = edge.lanes[l];
						lane.parent = &edge;
						lane.orientation = core::LaneOrientation::Backward;
						lane.width = way->laneWidth;
						double offset = (static_cast<double>(l) - (static_cast<double>(lane_count - 1) / 2.0)) * lane.width;
						Coordinates start = offset_coordinate(next->coordinates, heading, offset);
						Coordinates end = offset_coordinate(current->coordinates, heading, offset);
						lane.centerLine = { start, end };
						if (l < way->backwardTurns.size()) {
							lane.turn = way->backwardTurns[l];
						} else {
							lane.turn = core::TurnDirection::Straight;
						}
					}
					edge_graph_indices[next].push_back(network.edges.size() - 1);
				}

				current->tags = current->tags | NodeTags::Way;
				next->tags = current->tags | NodeTags::Way;
			}
		}

		for (const auto& [node, indices] : edge_graph_indices) {
			for (const auto& index : indices) {
				network.edge_graph[node].push_back(&network.edges[index]);
			}
		}
	}

} // namespace tjs::core::algo
