#include <core/stdafx.h>

#include <core/map_math/lane_connector_builder.h>
#include <core/map_math/earth_math.h>
#include <core/math_constants.h>

namespace tjs::core::algo {
	namespace {

		// Normalize angle to the [-180,180] range. This is used when
		// comparing headings between edges.
		double normalize_angle(double deg) noexcept
		{
			double x = std::fmod(deg + 180.0, 360.0);   // now in (-360, 360]
			if (x < 0) x += 360.0;                      // --> [0, 360)
			return x - 180.0;                           // --> [-180, 180)
		}

		// Forward declaration for helper below
		static bool is_link_type(WayType type);

		/// Returns a heading in degrees in the range [-180, 180],
		/// where 0° points due East ( +X ), positive angles turn CCW (towards +Y).
		static double heading_deg(const Coordinates& a, const Coordinates& b) noexcept
		{
			const double dx = b.x - a.x;
			const double dy = b.y - a.y;
			return std::atan2(dy, dx) * 180.0 / MathConstants::M_PI;   // atan2 already gives signed angle
		}

		namespace t {
			Coordinates operator - (const Coordinates& a, const Coordinates& b) {
				return {0, 0, a.x - b.x, a.y - b.y};
			}

			double signed_angle_deg(const Coordinates& v1, const Coordinates& v2)
			{
				double dot  = v1.x * v2.x + v1.y * v2.y;
				double det  = v1.x * v2.y - v1.y * v2.x;   // = |v1|·|v2|·sinθ
				return std::atan2(det, dot) * 180.0 / MathConstants::M_PI; // (-180,180]
			}

			TurnDirection classify(const Coordinates& a, const Coordinates& o,
                       const Coordinates& b, bool rhs = true)
			{
				Coordinates v_in  = a - o;    // back-wards so heading points *into* node
				Coordinates v_out = b - o;   // usual forward direction
				double θ = signed_angle_deg(v_in, v_out);

				if (std::abs(θ) <= 30.0)   return TurnDirection::Straight;
				if (θ  >  30.0 && θ <= 150.0) return rhs ? TurnDirection::Left  : TurnDirection::Right;
				if (θ  < -30.0 && θ >= -150.0) return rhs ? TurnDirection::Right : TurnDirection::Left;
				return TurnDirection::UTurn;
			}
		}



		// Determine the turn direction when travelling from in_edge to
		// out_edge based on their headings.
		static TurnDirection relative_direction(const Edge* in_edge,
                                        const Edge* out_edge,
                                        bool right_hand_traffic = true)
		{
			if (!in_edge || !out_edge) {
				return TurnDirection::None;
			}

			return t::classify(
				in_edge->start_node->coordinates,
				out_edge->start_node->coordinates,
				out_edge->end_node->coordinates
			);

			// 1. Signed angle between the edges ------------------------------------
			const double h_in  = heading_deg(in_edge->start_node->coordinates,
											in_edge->end_node->coordinates);
			const double h_out = heading_deg(out_edge->start_node->coordinates,
											out_edge->end_node->coordinates);

			double diff = h_out - h_in;                // raw difference
			diff = std::remainder(diff, 360.0);        // wrap to (-180, 180]

			// 2. Classify -----------------------------------------------------------
			const double abs_d = std::abs(diff);

			// (a) U-turn
			if (abs_d > 150.0) {
				return TurnDirection::UTurn;
			}

			// (b) Straight (allow small wiggle)
			if (abs_d <= 30.0) {
				// Optional: detect “ramp → mainline” merge
				if (is_link_type(in_edge->way->type) && !is_link_type(out_edge->way->type)) {
					return right_hand_traffic ? TurnDirection::MergeRight
											: TurnDirection::MergeLeft;
				}
				return TurnDirection::Straight;
			}

			// (c) Left vs Right
			if (diff > 0.0) {   // positive = CCW in our heading convention
				return right_hand_traffic ? TurnDirection::Left
										: TurnDirection::Right;
			}
			else {
				return right_hand_traffic ? TurnDirection::Right
										: TurnDirection::Left;
			}
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
			if (lane.turn == TurnDirection::Straight && (desired == TurnDirection::Straight || desired == TurnDirection::MergeRight || desired == TurnDirection::MergeLeft)) {
				return true;
			}
			if (lane.turn == TurnDirection::Right && desired == TurnDirection::MergeRight) {
				return true;
			}
			if (lane.turn == TurnDirection::Left && desired == TurnDirection::MergeLeft) {
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
					check_lane_dist(from_lane, out_lanes.back());
					return &out_lanes.back();
				}

				case TurnDirection::Left:
				case TurnDirection::MergeLeft: {
					check_lane_dist(from_lane, out_lanes.front());
					return &out_lanes.front();
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
				// TODO[simulation algo]: Connect all incoming lanes to the last outgoing lane
				// This should be deleted because no connection is really set and properely handled in algo
				if (incoming.front()->lanes.size() > outgoing.front()->lanes.size()) {
					Lane* to_lane = &outgoing.front()->lanes.back();
					for (size_t i = lanes; i < incoming.front()->lanes.size(); ++i) {
						Lane& from_lane = incoming.front()->lanes[i];
						network.lane_links.push_back({ &from_lane, to_lane, is_link_type(incoming.front()->way->type) });
						LaneLinkHandler link_handler { network.lane_links, network.lane_links.size() - 1 };
						from_lane.outgoing_connections.push_back(link_handler);
						to_lane->incoming_connections.push_back(link_handler);
						network.lane_graph[&from_lane].push_back(link_handler);
					}
				}
				continue;
			}

			// General case: connect every incoming lane to an
			// appropriate lane on each outgoing edge respecting
			// lane turn restrictions.
			size_t processed_lanes = 0;
			for (Edge* in_edge : incoming) {
				for (Edge* out_edge : outgoing) {
					TurnDirection desired = relative_direction(in_edge, out_edge);
					for (Lane& from_lane : in_edge->lanes) {
						if (!is_turn_allowed(from_lane, desired)) {
							continue;
						}
						Lane* to_lane = get_target_lane(from_lane, out_edge, desired, processed_lanes);
						if (!to_lane) {
							continue;
						}
						++processed_lanes;

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
