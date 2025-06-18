#include <core/stdafx.h>

#include <core/map_math/lane_connector_builder.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::algo {
	namespace {

		static double normalize_angle(double angle) {
			while (angle > 180.0) {
				angle -= 360.0;
			}
			while (angle < -180.0) {
				angle += 360.0;
			}
			return angle;
		}

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

	} // namespace

	void LaneConnectorBuilder::build_lane_connections(core::RoadNetwork& network) {
		// clear old connections
		for (auto& edge : network.edges) {
			for (auto& lane : edge.lanes) {
				lane.outgoing_connections.clear();
				lane.incoming_connections.clear();
			}
		}

		for (const auto& [nid, node] : network.nodes) {
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

			for (Edge* in_edge : incoming) {
				for (Edge* out_edge : outgoing) {
					TurnDirection desired = relative_direction(in_edge, out_edge);
					size_t count = std::min(in_edge->lanes.size(), out_edge->lanes.size());
					for (size_t i = 0; i < count; ++i) {
						Lane* from_lane = &in_edge->lanes[i];
						Lane* to_lane = &out_edge->lanes[i];
						if (!is_turn_allowed(*from_lane, desired)) {
							continue;
						}
						from_lane->outgoing_connections.push_back(to_lane);
						to_lane->incoming_connections.push_back(from_lane);
					}
				}
			}
		}
	}

} // namespace tjs::core::algo
