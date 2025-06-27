#include <core/stdafx.h>

#include <core/map_math/lane_connector_builder.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::algo {
	namespace {

		// Normalize angle to the [-180,180] range. This is used when
		// comparing headings between edges.
		static double normalize_angle(double angle) {
			while (angle > 180.0) {
				angle -= 360.0;
			}
			while (angle < -180.0) {
				angle += 360.0;
			}
			return angle;
		}

		// Determine the turn direction when travelling from in_edge to
		// out_edge based on their headings.
		static TurnDirection relative_direction(const Edge* in_edge, const Edge* out_edge) {
			double heading_in = bearing(in_edge->start_node->coordinates, in_edge->end_node->coordinates);
			double heading_out = bearing(out_edge->start_node->coordinates, out_edge->end_node->coordinates);
			double diff = normalize_angle(heading_out - heading_in);
			if (std::abs(diff) <= 30.0) {
				return TurnDirection::Straight;
			}
			if (diff > 30.0 && diff < 150.0) {
				return TurnDirection::Left;
			}
			if (diff < -30.0 && diff > -150.0) {
				return TurnDirection::Right;
			}
			return TurnDirection::UTurn;
		}

		// Check if the lane declaration explicitly forbids the desired
		// turning movement.
		static bool is_turn_allowed(const Lane& lane, TurnDirection desired) {
			if (lane.turn == TurnDirection::None) {
				return true;
			}
			if (lane.turn == desired) {
				return true;
			}
			if (lane.turn == TurnDirection::Straight && desired == TurnDirection::Straight) {
				return true;
			}
			return false;
		}

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

		// Find the most appropriate target lane on the outgoing edge for
		// a vehicle currently in `from_lane` that intends to perform the
		// specified `turn` movement.
		Lane* get_target_lane(Lane* from_lane, Edge* out_edge, TurnDirection turn) {
			auto& out_lanes = out_edge->lanes;
			if (out_lanes.empty()) {
				return nullptr;
			}

			switch (turn) {
				case TurnDirection::Straight:
					// Match by index if possible
					if (from_lane->parent && !from_lane->parent->lanes.empty()) {
						size_t from_index = std::distance(
							from_lane->parent->lanes.data(),
							from_lane);

						if (from_index < out_lanes.size()) {
							return &out_lanes[from_index];
						} else {
							// fallback to closest index
							return &out_lanes.back();
						}
					}
					break;

				case TurnDirection::Right:
					// Prefer rightmost lane
					return &out_lanes.back();

				case TurnDirection::Left:
					// Prefer leftmost lane
					return &out_lanes.front();

				case TurnDirection::UTurn:
					// Optional: handle separately
					return &out_lanes.front();

				default:
					break;
			}

			return nullptr;
		}

	} // namespace

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
			// Collect incoming and outgoing edges that touch this
			// node. We only consider edges with explicit start/end
			// pointers to this node.
			std::vector<Edge*> incoming;
			std::vector<Edge*> outgoing;
			for (auto& edge : network.edges) {
				if (edge.end_node == node) {
					incoming.push_back(&edge);
				}
				if (edge.start_node == node) {
					outgoing.push_back(&edge);
				}
			}

			// Special case: exactly one incoming and one outgoing
			// oneway edge. In this merge/split scenario we simply
			// connect lanes by their index so that each incoming
			// lane transitions to the corresponding outgoing lane.
			if (incoming.size() == 1 && outgoing.size() == 1 && !incoming.front()->lanes.empty() && !outgoing.front()->lanes.empty() && incoming.front()->way->isOneway && outgoing.front()->way->isOneway) {
				size_t lanes = std::min(incoming.front()->lanes.size(), outgoing.front()->lanes.size());
				for (size_t i = 0; i < lanes; ++i) {
					Lane& from_lane = incoming.front()->lanes[i];
					Lane& to_lane = outgoing.front()->lanes[i];
					network.lane_links.push_back({ &from_lane, &to_lane, is_link_type(incoming.front()->way->type) });
					LaneLinkHandler link_handler { network.lane_links, network.lane_links.size() - 1 };
					from_lane.outgoing_connections.push_back(link_handler);
					to_lane.incoming_connections.push_back(link_handler);
					network.lane_graph[&from_lane].push_back(link_handler);
				}
				continue;
			}

			// General case: connect every incoming lane to an
			// appropriate lane on each outgoing edge respecting
			// lane turn restrictions.
			for (Edge* in_edge : incoming) {
				for (Edge* out_edge : outgoing) {
					TurnDirection desired = relative_direction(in_edge, out_edge);
					for (Lane& from_lane : in_edge->lanes) {
						if (!is_turn_allowed(from_lane, desired)) {
							continue;
						}

						Lane* to_lane = get_target_lane(&from_lane, out_edge, desired);
						if (!to_lane) {
							continue;
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
	}

} // namespace tjs::core::algo
