#include <core/stdafx.h>

#include <core/map_math/lane_connector_builder.h>
#include <core/map_math/earth_math.h>
#include <core/math_constants.h>

namespace tjs::core::algo {
	namespace {
		// Forward declaration for helper below
		// Helper to detect if a way is a *_link highway. Those links are
		// considered yielding when merging.
		static bool is_link_type(WayType type) {
			switch (type) {
				case WayType::MotorwayLink:
				case WayType::TrunkLink:
				case WayType::PrimaryLink:
				case WayType::SecondaryLink:
				case WayType::TertiaryLink:
					return true;
				default:
					return false;
			}
		}

		// Determine the turn direction when travelling from in_edge to
		// out_edge based on their headings.
		static TurnDirection relative_direction(const Edge* in_edge,
			const Edge* out_edge,
			bool right_hand_traffic = true) {
			if (!in_edge || !out_edge) {
				return TurnDirection::None;
			}

			return get_relative_direction(
				in_edge->start_node->coordinates,
				out_edge->start_node->coordinates,
				out_edge->end_node->coordinates);
		}

		// Check if the lane declaration explicitly forbids the desired
		// turning movement.
		static bool is_turn_allowed(const Lane& lane, TurnDirection desired) {
			if (lane.turn == TurnDirection::None) {
				size_t from_index = std::distance(
					lane.parent->lanes.data(),
					const_cast<Lane*>(&lane));

				if (desired == TurnDirection::UTurn) {
					// forbid auto U-turn for now
					return false;
				}
				if (desired == TurnDirection::Left) {
					// Left turn is only allowed on the last lane
					return from_index == (lane.parent->lanes.size() - 1);
				} else if (desired == TurnDirection::Right) {
					// Right turn is only allowed on the first lane
					return from_index == 0;
				}

				return true;
			}
			return has_flag(lane.turn, desired);
		}

		// Find the most appropriate target lane on the outgoing edge for
		// a vehicle currently in `from_lane` that intends to perform the
		// specified `turn` movement.
		// For merging roads, when there are several incomings it should take offset for each road
		//   processed_lanes is roads that already are marked
		Lane* get_target_lane(Lane& from_lane, Edge* out_edge, TurnDirection turn, size_t processed_lanes) {
			auto& out_lanes = out_edge->lanes;
			if (out_lanes.empty()) {
				return nullptr;
			}

			auto check_lane_dist = [](const Lane& from_lane, const Lane& to_lane) {
				const double dist = euclidean_distance(from_lane.centerLine.back(), to_lane.centerLine.front());
				if (dist > 1.0) {
					// TODO: algo error handling
					return false;
				}
				return true;
			};

			switch (turn) {
				case TurnDirection::Right:
				case TurnDirection::MergeRight: {
					check_lane_dist(from_lane, out_lanes.front());
					return &out_lanes.front();
				}

				case TurnDirection::Left:
				case TurnDirection::MergeLeft: {
					check_lane_dist(from_lane, out_lanes.back());
					return &out_lanes.back();
				}

				default:
					break;
			}

			// Straight/UTurn or fallback: choose lane based on lateral position
			size_t out_index = processed_lanes;
			if (out_index < out_lanes.size() - 1) {
				check_lane_dist(from_lane, out_lanes[out_index]);
				return &out_lanes[out_index];
			}

			// If there are less lanes in out, use the last one
			// TODO: If there are many lanes should mark somehow so algo will merge earlier
			check_lane_dist(from_lane, out_lanes.back());
			return &out_lanes.back();
		}

	} // namespace

	details::AdjacentEdges details::get_adjacent_edges(RoadNetwork& network, Node* node) {
		AdjacentEdges adjacent;

		auto& incoming = adjacent.incoming;
		auto& outgoing = adjacent.outgoing;
		WayType min_priority = WayType::Count;

		for (auto& edge : network.edges) {
			if (edge.end_node == node) {
				incoming.push_back(&edge);
				if (edge.way->type < min_priority) {
					min_priority = edge.way->type;
					adjacent.primary = &edge;
				}
			}
			if (edge.start_node == node) {
				outgoing.push_back(&edge);
			}
		}

		if (incoming.size() == 0) {
			return adjacent;
		}

		// if the road is upward, the rightest will be with max x coordinate
		const bool road_goes_up = adjacent.primary->start_node->coordinates.y < adjacent.primary->end_node->coordinates.y;
		// sort incoming by start node (end is the same)
		std::ranges::sort(incoming, [road_goes_up](Edge* a, Edge* b) {
			if (road_goes_up) {
				return a->start_node->coordinates.x > b->start_node->coordinates.x;
			}
			return a->start_node->coordinates.x < b->start_node->coordinates.x;
		});
		// sort outgoin by end_node (start is the same)
		std::ranges::sort(outgoing, [road_goes_up](Edge* a, Edge* b) {
			if (road_goes_up) {
				return a->end_node->coordinates.x > b->end_node->coordinates.x;
			}
			return a->end_node->coordinates.x < b->end_node->coordinates.x;
		});

		return adjacent;
	}

	void details::process_node(RoadNetwork& network, Node* node) {
		// Collect incoming and outgoing edges that touch this
		// node. We only consider edges with explicit start/end
		// pointers to this node.

		auto adjacent = get_adjacent_edges(network, node);

		std::vector<Edge*>& incoming = adjacent.incoming;
		std::vector<Edge*>& outgoing = adjacent.outgoing;
		Edge* primary = adjacent.primary;

		// General case: connect every incoming lane to an
		// appropriate lane on each outgoing edge respecting
		// lane turn restrictions.
		std::unordered_map<Edge*, size_t> outgoing_processed;
		for (Edge* edge : outgoing) {
			outgoing_processed[edge] = 0;
		}

		// Merging lanes logic:
		// When incoming_lanes < outgoing_lanes:
		//   incoming connects to outgoing from right
		// When incoming_lanes == outgoing_lanes:
		//   incoming connects to outgoing from right
		// When incoming_lanes > outgoing_lanes:
		//   primary incoming connects to outgoing from right
		//   secondary incoming connects from primary's corner

		for (Edge* in_edge : incoming) {
			for (Edge* out_edge : outgoing) {
				size_t& processed_lanes = outgoing_processed[out_edge];
				TurnDirection desired = relative_direction(in_edge, out_edge);
				// adjust first lane index for primary
				const bool is_primary = in_edge == primary;
				if (is_primary) {
					int needed_lanes = 0;
					for (Lane& from_lane : in_edge->lanes) {
						if (!is_turn_allowed(from_lane, desired)) {
							continue;
						}
						++needed_lanes;
					}
					size_t candidate_out_idx = 0;
					if (processed_lanes + needed_lanes <= out_edge->lanes.size()) {
						candidate_out_idx = processed_lanes;
					} else {
						candidate_out_idx = processed_lanes;
						while (candidate_out_idx != 0 && (candidate_out_idx + needed_lanes) >= out_edge->lanes.size()) {
							--candidate_out_idx;
						}
						processed_lanes = candidate_out_idx;
					}
				}

				for (Lane& from_lane : in_edge->lanes) {
					const bool merging_lane = has_flag(from_lane.turn, TurnDirection::MergeLeft) || has_flag(from_lane.turn, TurnDirection::MergeRight);
					if (!merging_lane && !is_turn_allowed(from_lane, desired)) {
						// check this is not merging
						continue;
					}
					Lane* to_lane = get_target_lane(from_lane, out_edge, desired, processed_lanes);
					if (!to_lane) {
						continue;
					}

					auto it = std::ranges::find_if(from_lane.outgoing_connections, [to_lane](const LaneLinkHandler& link) {
						return link->to == to_lane;
					});
					if (it != from_lane.outgoing_connections.end()) {
						continue;
					}

					// TODO: adjust lane
					const bool turning_lane = has_flag(from_lane.turn, TurnDirection::Left) || has_flag(from_lane.turn, TurnDirection::Right);
					if (turning_lane) {
						//to_lane->centerLine.front() = from_lane.centerLine.back();
					} else {
						//from_lane.centerLine.back() = to_lane->centerLine.front();
					}
					// in one way can be several edges with one turn direction
					if (!merging_lane || to_lane->turn == from_lane.turn) {
						++processed_lanes;
					}

					network.lane_links.push_back({ &from_lane, to_lane, is_link_type(in_edge->way->type) });
					LaneLinkHandler link_handler { network.lane_links, network.lane_links.size() - 1 };
					from_lane.outgoing_connections.push_back(link_handler);
					to_lane->incoming_connections.push_back(link_handler);
					network.lane_graph[&from_lane].push_back(link_handler);
				}
			}
		}
	}

	void LaneConnectorBuilder::build_lane_connections(core::RoadNetwork& network) {
		// Remove any previously generated links so the builder can be
		// called multiple times without leaking connections.
		for (auto& edge : network.edges) {
			for (auto& lane : edge.lanes) {
				lane.outgoing_connections.clear();
				lane.incoming_connections.clear();
			}
		}
		network.lane_links.clear();
		network.lane_graph.clear();

		for (const auto& [nid, node] : network.nodes) {
			details::process_node(network, node);
		}
	}

} // namespace tjs::core::algo
