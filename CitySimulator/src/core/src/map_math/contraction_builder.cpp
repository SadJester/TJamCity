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
			orientation == LaneOrientation::Forward ? way->lanesForward : way->lanesBackward);

		const size_t total_lanes = way->lanesForward + way->lanesBackward;
		const double half_lanes_offset = (total_lanes - 1) / 2.0;

		edge.lanes.resize(reserved_size);

		// ------------------------------------------------------------------ //
		// 1. Geometric basis                                                 //
		// ------------------------------------------------------------------ //
		double heading = bearing(start_node->coordinates, end_node->coordinates);

		const double x_delta = std::fabs(end_node->coordinates.x - start_node->coordinates.x);
		const double y_delta = std::fabs(end_node->coordinates.y - start_node->coordinates.y);
		const bool decision_by_y = x_delta < y_delta;
		bool right_is_min = decision_by_y ? start_node->coordinates.y > end_node->coordinates.y : start_node->coordinates.x > end_node->coordinates.x;
		edge.opposite_side = right_is_min ? Edge::OppositeSide::Right : Edge::OppositeSide::Left;
		size_t adjacent_lane_offset = 0;
		if (orientation == LaneOrientation::Backward) {
			heading = bearing(end_node->coordinates, start_node->coordinates);
			edge.opposite_side = right_is_min ? Edge::OppositeSide::Left : Edge::OppositeSide::Right;
			right_is_min = !right_is_min;
		}

		if (decision_by_y) {
			edge.opposite_side = edge.opposite_side == Edge::OppositeSide::Right ? Edge::OppositeSide::Left : Edge::OppositeSide::Right;
		}

		if (
			(!right_is_min && orientation == LaneOrientation::Forward)
			|| (right_is_min && orientation == LaneOrientation::Backward)) {
			adjacent_lane_offset = total_lanes - reserved_size;
		}

		// ------------------------------------------------------------------ //
		// 2. Fill lanes – **index 0 = right-most**                           //
		// ------------------------------------------------------------------ //
		for (size_t logical_idx = 0; logical_idx < reserved_size; ++logical_idx) {
			// ★ Physical index so that 0 = right-most,   N-1 = left-most
			size_t idx = logical_idx;
			if (orientation == LaneOrientation::Backward || (!decision_by_y && orientation == LaneOrientation::Forward)) {
				idx = reserved_size - 1 - logical_idx;
			}
			size_t offset = idx + adjacent_lane_offset;

			Lane& lane = edge.lanes[logical_idx];

			lane.index_in_edge = logical_idx;
			lane.parent = &edge;
			lane.orientation = orientation;
			lane.width = way->laneWidth;

			// Offset sign is + to the **left** of travel direction.
			// Therefore right-most lane has the **most negative** offset.
			const double lateral_offset = (offset - half_lanes_offset) * way->laneWidth;

			Coordinates start = offset_coordinate(start_node->coordinates, heading, lateral_offset);
			Coordinates end = offset_coordinate(end_node->coordinates, heading, lateral_offset);

			lane.centerLine = { start, end };
			lane.rotation_angle = static_cast<float>(atan2(end.y - start.y, end.x - start.x));
			lane.length = euclidean_distance(start, end);

			// ----------------------------------------------------------------//
			// 3. Per-lane turn indication                                      //
			// ----------------------------------------------------------------//
			TurnDirection td = TurnDirection::None;
			// For forward directions is always reversed idx
			// For backward is the same as in OSM format
			if (orientation == LaneOrientation::Forward) {
				size_t td_idx = reserved_size - 1 - logical_idx;
				if (td_idx < way->forwardTurns.size()) {
					td = way->forwardTurns[td_idx];
				}
			} else { // Backward
				size_t td_idx = reserved_size - 1 - logical_idx;
				if (td_idx < way->backwardTurns.size()) {
					td = way->backwardTurns[td_idx];
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
					way->edges.push_back({ EdgeHandler { network.edges, network.edges.size() - 1 } });
					edge_graph_indices[current].push_back(network.edges.size() - 1);
				}

				if (!way->isOneway && way->lanesBackward > 0) {
					network.edges.push_back(create_edge(next, current, way, dist, LaneOrientation::Backward));
					way->edges.push_back({ EdgeHandler { network.edges, network.edges.size() - 1 } });
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
