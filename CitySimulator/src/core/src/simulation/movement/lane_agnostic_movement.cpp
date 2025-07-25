#include <core/stdafx.h>

#include <core/simulation/movement/lane_agnostic_movement.h>
#include <core/simulation/transport_management/vehicle_state.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/edge.h>
#include <core/data_layer/vehicle.h>

#include <core/simulation/agent/agent_data.h>

namespace tjs::core::simulation {

	// ------------------------------------------------------------------
	// Build a 32-bit mask for *current* edge that flags which lanes can
	// exit into `next_edge` according to LaneLink table.
	// ------------------------------------------------------------------
	uint32_t build_goal_mask(const Edge& curr_edge, const Edge& next_edge) {
		uint32_t mask = 0;

		for (size_t i = 0; i < curr_edge.lanes.size(); ++i) {
			const Lane& ln = curr_edge.lanes[i];
			size_t from_idx = i;

			for (LaneLinkHandler h : ln.outgoing_connections) {
				const LaneLink& link = *h;
				if (link.to && link.to->parent == &next_edge) {
					mask |= (1u << from_idx); // mark this *source* lane
					break;                    // no need to scan others
				}
			}
		}
		return mask; // 0 means “none of the lanes reach next_edge” → error
	}

	bool gap_ok(Lane* lane, size_t row) {
		return true;
	}

	float idm_scalar(float s, float v) {
		return 0.4f;
	}

	void phase1_simd(
		const std::vector<AgentData>& agents,
		VehicleBuffers& buf, // one object, not “in/out”
		const std::vector<LaneRuntime>& lane_rt,
		double dt) {
		TJS_TRACY_NAMED("VehicleMovement_Phase1");
		/* threaded outer loop over lanes */

		static constexpr float T_MIN = 0.f;
		static constexpr float D_PREP = 80.0f;

		// #pragma omp parallel for schedule(dynamic,4)
		for (std::size_t L = 0; L < lane_rt.size(); ++L) {
			const LaneRuntime& rt = lane_rt[L];
			const auto* idx = rt.idx.data();
			const std::size_t n = rt.idx.size();
			const float lane_length = rt.length;

			/* scalar inner loop for clarity — replace with gather/SIMD later */
			for (std::size_t k = 0; k < n; ++k) {
				std::size_t i = idx[k];

				if (buf.flags[i] & FL_ERROR) {
					continue;
				}

				//----------------- longitudinal IDM ----------------------
				float v = buf.v_curr[i];
				double s = buf.s_curr[i];
				float a = idm_scalar(s, v /* + leader row idx[k-1] */);

				const float v_next = v + a * dt;
				// TODO[simulation]: clamp speed based on vehicle and way
				const float max_speed = rt.max_speed;
				buf.v_next[i] = v_next > max_speed ? max_speed : v_next;
				buf.s_next[i] = s + v * dt + 0.5 * a * dt * dt;

				// ─── 2. lane-change decision (simplified) ─────────────
				double dist = rt.length - s; // to node
				bool near = dist < D_PREP;

				if (near && buf.lane_target[i] == nullptr) {
					// TODO: must be removed from simd?
					const Lane* lane = rt.static_lane;
					const AgentData& ag = agents[i];
					bool ok = (ag.goal_lane_mask >> lane->index_in_edge) & 1;

					if (ok && (buf.flags[i] & FL_COOLDOWN) == 0) {
						Lane* left = lane->left();   // lane->turn == TurnDirection::Left  ? nullptr : lane->parent->lanes[lane->index_in_edge-1];
						Lane* right = lane->right(); //lane->turn == TurnDirection::Right ? nullptr : lane->parent->lanes[lane->index_in_edge+1];

						auto want = [&](Lane* L) {
							return L && ((ag.goal_lane_mask >> L->index_in_edge) & 1);
						};

						if (want(left) /*&& gap_ok(left, k, rt, buf)*/) {
							buf.lane_target[i] = left;
						} else if (want(right) /*&& gap_ok(right, k, rt, buf)*/) {
							buf.lane_target[i] = right;
						}

						if (buf.lane_target[i]) { // scheduled?
							set_state(buf.flags[i], ST_EXECUTE);
						}
					}
				}

				//----------------- cool-down bookkeeping -----------------
				if (buf.flags[i] & FL_COOLDOWN) {
					/* crude scalar timer in seconds */
					float t = buf.lateral_off[i]; // repurpose column or add timer col.
					t += dt;
					if (t > T_MIN) {
						buf.flags[i] &= ~FL_COOLDOWN;
						t = 0.f;
					}
					buf.lateral_off[i] = t;
				}
			}
		}
	}

	//------------------------------------------------------------------
	//  Pick the concrete landing lane on `next_edge` for a car that is
	//  currently in `src_lane` and about to cross the node.
	//
	//  • Must return a valid pointer (throws if the route is impossible).
	//  • Preference order:   1) non-yield link
	//                        2) shortest lateral hop (|id diff|)
	//                        3) first found
	//------------------------------------------------------------------
	inline Lane* choose_entry_lane(const Lane* src_lane, const Edge* next_edge, MovementError& err) {
		Lane* best = nullptr;
		bool best_is_yield = true; // so non-yield wins
		int best_shift = INT_MAX;  // minimise |Δlane|

		const std::size_t src_idx = src_lane->index_in_edge; // local index in its edge
		err = MovementError::None;
		for (const LaneLinkHandler& h : src_lane->outgoing_connections) {
			const LaneLink& link = *h;
			Lane* tgt = link.to;
			// wrong edge
			if (!tgt || tgt->parent != next_edge) {
				continue;
			}

			bool is_yield = link.yield;
			int shift = std::abs(int(tgt->index_in_edge) - int(src_idx));

			if (!best || (best_is_yield && !is_yield) || (is_yield == best_is_yield && shift < best_shift)) {
				best = tgt;
				best_is_yield = is_yield;
				best_shift = shift;
			}
		}

		if (src_lane->outgoing_connections.empty()) {
			err = MovementError::NoOutgoingConnections;
			return nullptr;
		}
		if (!best) {
			bool has_connection = false;
			for (auto& lane : src_lane->parent->lanes) {
				if (&lane == src_lane) {
					continue;
				}
				for (auto& link : lane.outgoing_connections) {
					if (link->to->parent == next_edge) {
						has_connection = true;
						break;
					}
				}
				if (has_connection) {
					break;
				}
			}

			// If has connection we are on the wrong lane, if not - totally wrong edge (how we get here?)
			err = has_connection ? MovementError::IncorrectLane : MovementError::IncorrectEdge;
			// TODO[simulation]: algo error handling
			//throw std::runtime_error(
			//	"Route impossible: no LaneLink from edge " + std::to_string(src_lane->parent->get_id()) + " to edge " + std::to_string(next_edge->get_id()) + '.');
			return src_lane->outgoing_connections[0]->to;
		}
		err = MovementError::None;
		return best;
	}

	void move_index(std::size_t row,
		std::vector<LaneRuntime>& lane_rt,
		const Lane* src,
		const Lane* tgt,
		const std::vector<double>& s_curr) {
		// nothing to do
		if (src == tgt) {
			return;
		}

		auto& v_src = lane_rt[src->index_in_buffer].idx;
		auto& v_tgt = lane_rt[tgt->index_in_buffer].idx;

		/* ---- 1. O(1) erase from source by swap-and-pop ---------------- */
		{
			auto it = std::find(v_src.begin(), v_src.end(), row);
			if (it != v_src.end()) {
				std::iter_swap(it, v_src.end() - 1);
				v_src.pop_back();
			}
		}

		/* ---- 2. O(log N) insert into target, keep sorted -------------- */
		const double s = s_curr[row];
		auto it_ins = std::lower_bound(
			v_tgt.begin(), v_tgt.end(), s,
			[&](std::size_t j, double pos) { return s_curr[j] > pos; });

		v_tgt.insert(it_ins, row);
	}

	void phase2_commit(
		std::vector<AgentData>& agents,
		VehicleBuffers& buf,
		std::vector<LaneRuntime>& lane_rt) {
		TJS_TRACY_NAMED("VehicleMovement_Phase2");
		/* -------- (i) swap snapshot ---------------------------------- */
		buf.s_curr.swap(buf.s_next);
		buf.v_curr.swap(buf.v_next);

		/* -------- (ii) overshoot & route advance --------------------- */
		for (size_t i = 0; i < agents.size(); ++i) {
			AgentData& ag = agents[i];

			if (ag.path.empty()) {
				continue;
			}

			double remain = buf.s_curr[i];
			Lane* lane = buf.lane[i];

			while (remain >= lane->length - 1e-6) {
				remain -= lane->length;
				/* advance path */
				++ag.path_offset;
				if (ag.path_offset >= ag.path.size()) {             // finished trip
					move_index(i, lane_rt, lane, lane, buf.s_curr); // erase only
					set_state(buf.flags[i], ST_FOLLOW);             // idle state
					buf.flags[i] |= FL_ERROR;
					ag.vehicle->state = VehicleState::Stopped;
					ag.vehicle->error = MovementError::NoPath;
					buf.s_curr[i] = lane->length;
					buf.s_next[i] = buf.s_curr[i];
					goto next_vehicle; // despawn else
				}
				Edge* next_edge = ag.path[ag.path_offset];
				MovementError err;
				Lane* entry = choose_entry_lane(lane, next_edge, err);

				if (err != MovementError::None) {
					ag.vehicle->error = err;
					ag.vehicle->state = VehicleState::Stopped;
				} else {
					ag.goal_lane_mask = build_goal_mask(*entry->parent, *ag.path[ag.path_offset + 1]);
				}

				if (entry != nullptr) {
					move_index(i, lane_rt, lane, entry, buf.s_curr);
					lane = entry;
					buf.lane[i] = entry;
				} else {
					goto next_vehicle; // despawn else
				}
			}
			buf.s_curr[i] = remain;

		next_vehicle:;
		}

		/* -------- (iii) lateral commitments -------------------------- */
		for (LaneRuntime& rt : lane_rt) {
			/* iterate copy because rt.idx will mutate when we move rows */
			auto idx_copy = rt.idx;
			for (std::size_t row : idx_copy) {
				Lane* tgt = buf.lane_target[row];
				if (!tgt) {
					continue;
				}

				move_index(row, lane_rt, rt.static_lane, tgt, buf.s_curr);
				buf.lane[row] = tgt;
				buf.lane_target[row] = nullptr;
				set_state(buf.flags[row], ST_FOLLOW);
				buf.flags[row] |= FL_COOLDOWN; // start cool-down
			}
		}
	}

} // namespace tjs::core::simulation
