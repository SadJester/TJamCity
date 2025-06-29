#include <core/stdafx.h>

#include <core/map_math/contraction_builder.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::algo {

	Edge create_edge(Node* start_node,
		Node* end_node,
		WayInfo* way,
		double dist,
		core::LaneOrientation orientation) {
		Edge edge;
		edge.start_node = start_node;
		edge.end_node = end_node;
		edge.way = way;
		edge.orientation = orientation;
		edge.length = dist;

		// At least one lane even if OSM data were incomplete
		const size_t reserved_size = std::max<std::size_t>(
			1,
			orientation == LaneOrientation::Forward ?
				way->lanesForward :
				way->lanesBackward);

		edge.lanes.resize(reserved_size);

		// ------------------------------------------------------------------ //
		// 1. Geometric basis                                                 //
		// ------------------------------------------------------------------ //
		const double heading = bearing(start_node->coordinates, end_node->coordinates);

		// Pre-compute half-width offset of the lane bundle
		const double half_span = reserved_size % 2 == 0 ?  0.5 * way->laneWidth : 0.0;
		// ------------------------------------------------------------------ //
		// 2. Fill lanes – **index 0 = right-most**                           //
		// ------------------------------------------------------------------ //
		bool right_is_min = orientation == LaneOrientation::Forward && start_node->coordinates.y < end_node->coordinates.y;
		for (size_t logical_idx = 0; logical_idx < reserved_size; ++logical_idx) {
			// ★ Physical index so that 0 = right-most,   N-1 = left-most
			size_t idx = right_is_min ? logical_idx : reserved_size - 1 - logical_idx;

			Lane& lane = edge.lanes[idx];

			lane.parent = &edge;
			lane.orientation = orientation;
			lane.width = way->laneWidth;

			// Offset sign is + to the **left** of travel direction.
			// Therefore right-most lane has the **most negative** offset.
			double lateral_offset = (static_cast<double>(logical_idx) - (reserved_size - 1) / 2) * way->laneWidth;
			lateral_offset -= half_span;
			// Reverse sign for Backward edges so that right side stays right.
			if (orientation == LaneOrientation::Backward) {
				lateral_offset = -lateral_offset; // keep RH convention
			}

			Coordinates start = offset_coordinate(start_node->coordinates,
				heading,
				lateral_offset);
			Coordinates end = offset_coordinate(end_node->coordinates,
				heading,
				lateral_offset);

			lane.centerLine = { start, end };
			lane.length = euclidean_distance(start, end);

			// ----------------------------------------------------------------//
			// 3. Per-lane turn indication                                      //
			// ----------------------------------------------------------------//
			TurnDirection td = TurnDirection::None;

			if (orientation == LaneOrientation::Forward) { // ★ fixed list swap
				if (logical_idx < way->forwardTurns.size()) {
					td = way->forwardTurns[logical_idx];
				}
			} else { // Backward
				if (logical_idx < way->backwardTurns.size()) {
					td = way->backwardTurns[logical_idx];
				}
			}
			lane.turn = td;
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
