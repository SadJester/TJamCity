#include <core/stdafx.h>

#include <core/map_math/contraction_builder.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::algo {

	Edge create_edge(Node* start_node, Node* end_node, WayInfo* way, double dist, core::LaneOrientation orientation) {
		Edge edge;
		edge.start_node = start_node;
		edge.end_node = end_node;
		edge.way = way;
		edge.orientation = orientation;
		edge.length = dist;

		const size_t reserved_size = orientation == LaneOrientation::Forward ? way->lanesForward : way->lanesBackward;
		edge.lanes.resize(reserved_size);

		double heading = bearing(start_node->coordinates, end_node->coordinates);
		const size_t lane_count = edge.lanes.size();
		for (size_t l = 0; l < reserved_size; ++l) {
			auto& lane = edge.lanes[l];
			lane.parent = nullptr;
			lane.orientation = orientation;
			lane.width = way->laneWidth;
			double offset = (static_cast<double>(l) - (static_cast<double>(lane_count - 1) / 2.0)) * lane.width;
			Coordinates start = offset_coordinate(start_node->coordinates, heading, offset);
			Coordinates end = offset_coordinate(end_node->coordinates, heading, offset);
			lane.centerLine = { start, end };
			lane.length = euclidean_distance(start, end);

			auto turn_direction = core::TurnDirection::None;
			if (orientation == LaneOrientation::Backward) {
				if (l < way->forwardTurns.size()) {
					turn_direction = way->forwardTurns[l];
				}
			}
			else if (orientation == LaneOrientation::Forward) {
				if (l < way->backwardTurns.size()) {
					turn_direction = way->backwardTurns[l];
				}
			}
			lane.turn = turn_direction;
		}
		return edge;
	}

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
				double dist = euclidean_distance(current->coordinates, next->coordinates);

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
					network.edges.push_back(create_edge(current, next, way, dist, LaneOrientation::Forward));
					edge_graph_indices[current].push_back(network.edges.size() - 1);
				}

				if (!way->isOneway && way->lanesBackward > 0) {
					network.edges.push_back(create_edge(next, current, way, dist, LaneOrientation::Backward));
					edge_graph_indices[next].push_back(network.edges.size() - 1);
				}

				current->tags = current->tags | NodeTags::Way;
				next->tags = current->tags | NodeTags::Way;
			}
		}


		for (auto& edge : network.edges) {
			for (auto& lane : edge.lanes) {
				lane.parent = &edge;
			}
		}

		for (const auto& [node, indices] : edge_graph_indices) {
			for (const auto& index : indices) {
				network.edge_graph[node].push_back(&network.edges[index]);
			}
		}
	}

} // namespace tjs::core::algo
