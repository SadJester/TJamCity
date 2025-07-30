#include <core/stdafx.h>

#include <core/simulation/movement/lane_agnostic_movement.h>
#include <core/simulation/transport_management/vehicle_state.h>

#include <core/data_layer/lane.h>
#include <core/data_layer/edge.h>
#include <core/data_layer/vehicle.h>

#include <core/simulation/agent/agent_data.h>
#include <core/simulation/simulation_system.h>

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

	namespace sim {

		struct idm_params_t {
			float s0 = 2.0f;         // minimum jam distance [m]
			float t_headway = 1.5f;  // safe time headway [s]
			float a_max = 1.0f;      // maximum acceleration [m/s²]
			float b_comf = 2.0f;     // comfortable deceleration [m/s²] (positive)
			float b_hard = 7.5f;     // max deceleration [m/s²] (positive)
			float v_desired = 30.0f; // desired cruise speed [m/s]
			float delta = 4.0f;      // acceleration exponent (4 = standard IDM)
		};

		// -----------------------------------------------------------------------------
		// GAP HELPERS
		// -----------------------------------------------------------------------------

		inline float safe_entry_speed(const float v_leader,
			const float gap,
			const double dt) noexcept {
			// Prevent division by zero when dt≈0
			if (dt <= 1e-6) {
				return v_leader;
			}
			// Allow the follower to close the entire gap within the next tick
			return v_leader + gap / static_cast<float>(dt);
		}

		// Physical bumper‑to‑bumper gap between follower and its leader.
		// Negative/zero gaps are clamped to 0 to avoid division by zero downstream.
		inline float actual_gap(const float s_leader_ctr,
			const float s_follower_ctr,
			const float len_leader,
			const float len_follower) noexcept {
			const float bumper_dist =
				(s_leader_ctr - s_follower_ctr) - 0.5f * (len_leader + len_follower);
			return std::max(0.0f, bumper_dist);
		}

		// Desired dynamic gap s* (IDM eq. 3) that keeps time‑headway and braking distance.
		// delta_v = v_follower - v_leader.  Positive when closing in.
		inline float desired_gap(const float v_follower,
			const float delta_v,
			const idm_params_t& p) noexcept {
			const float braking_term = (v_follower * delta_v) / (2.0f * std::sqrt(p.a_max * p.b_comf));
			const float dyn = v_follower * p.t_headway + braking_term;
			return p.s0 + std::max(0.0f, dyn);
		}

		// -----------------------------------------------------------------------------
		// IDM ACCELERATION (SCALAR REFERENCE)
		// -----------------------------------------------------------------------------

		// Scalar Intelligent‑Driver‑Model acceleration.  Used by unit tests and as a
		// readable reference for the SIMD drop‑in.
		//
		//   v_follower – current speed of vehicle [m/s]
		//   v_leader   – speed of leader [m/s]
		//   s_gap      – actual bumper‑to‑bumper distance [m]
		//   p          – calibrated IDM parameters
		inline float idm_scalar(const float v_follower,
			const float v_leader,
			const float s_gap,
			const idm_params_t& p) noexcept {
			const float delta_v = v_follower - v_leader; // closing speed
			const float s_star = desired_gap(v_follower, delta_v, p);

			// Free‑road acceleration and interaction (car‑following) terms
			const float term_free = std::pow(v_follower / p.v_desired, p.delta);
			const float term_int = std::pow(s_star / std::max(1e-3f, s_gap), 2.0f);

			// IDM equation
			float a = p.a_max * (1.0f - term_free - term_int);

			/* ---------- braking envelope ------------------------------------ */
			//  | a_max |        normal IDM
			//  |       |________________________________
			// 0|                                   .
			//  |                                   .
			//  |-b_comf|---- comfortable driving ---.
			//  |-b_hard| emergency only (collision!)|
			//
			if (a < -p.b_comf) {
				// Let the model go as far as -b_hard,
				// but never *increase* braking beyond what IDM asked for
				a = std::max(a, -p.b_hard);
			}
			return a;
		}

	} // namespace sim

	void phase1_simd(
		TrafficSimulationSystem& system,
		VehicleBuffers& buf,
		const std::vector<LaneRuntime>& lane_rt,
		const double dt) {
		TJS_TRACY_NAMED("VehicleMovement::IDM::Phase1");

		const auto& agents = system.agents();

		static constexpr float D_PREP_PER_LANE = 60.0f; // extra [m] per missing lane
		static constexpr float D_PREP = 80.0f;          // distance to start lane‑prep [m]
		static constexpr float T_MIN = 1.5f;            // cool‑down expiry threshold [s]

#if TJS_SIMULATION_DEBUG
		auto& debug = system.settings().debug_data;
#endif

		const sim::idm_params_t idm_def {}; // default calibrated parameters

		/* threaded outer loop over lanes (keep free to add OpenMP/TBB) */
		// #pragma omp parallel for schedule(dynamic,4)
		for (std::size_t L = 0; L < lane_rt.size(); ++L) {
			const LaneRuntime& rt = lane_rt[L];
			const auto* idx = rt.idx.data(); // sorted rear→front indices
			const std::size_t n = rt.idx.size();

			TJS_BREAK_IF(
				debug.movement_phase == SimulationMovementPhase::IDM_Phase1_Lane
				&& debug.lane_id == rt.static_lane->get_id()
				&& debug.vehicle_indices == rt.idx);

			// ---------------------------------------------------------------------
			// Scalar inner loop – one follower row at a time (will become gather/SIMD)
			// ---------------------------------------------------------------------
			for (std::size_t k = 0; k < n; ++k) {
				const std::size_t i = idx[k]; // follower row id

				TJS_BREAK_IF(
					debug.movement_phase == SimulationMovementPhase::IDM_Phase1_Vehicle
					&& debug.lane_id == rt.static_lane->get_id()
					&& debug.vehicle_indices.size() == 1
					&& k == debug.vehicle_indices[0]);

				// Skip broken cars
				if (VehicleStateBitsV::has_info(buf.flags[i], VehicleStateBits::FL_ERROR)) {
					continue;
				}

				// ─── 1. Gather follower state ────────────────────────────────────
				const float s_f = buf.s_curr[i]; // [m]
				const float v_f = buf.v_curr[i]; // [m/s]
				const float l_f = buf.length[i]; // bumper‑to‑bumper length [m]

				// ─── 1b. Gather leader state (if any) ────────────────────────────
				float s_gap = 1e9f;   // sentinel = "free road"
				float v_leader = v_f; // same speed → Δv = 0

				if (k > 0) {
					const size_t j = idx[k - 1];
					const float s_l = buf.s_curr[j];
					v_leader = buf.v_curr[j];

					const float s_l_ctr = buf.s_curr[j];
					const float v_leader = buf.v_curr[j];
					const float len_leader = buf.length[j];

					s_gap = sim::actual_gap(s_l_ctr, s_f, len_leader, l_f);
				}

				// ─── 2. IDM acceleration ────────────────────────────────────────
				const float a = sim::idm_scalar(v_f, v_leader, s_gap, idm_def);

				// ─── 3. Kinematics update (Euler forward) ────────────────────────
				const float v_next = std::clamp(v_f + a * static_cast<float>(dt), 0.0f, rt.max_speed);
				buf.v_next[i] = v_next;
				buf.s_next[i] = s_f + v_f * dt + 0.5f * a * static_cast<float>(dt * dt);

				// ─── 4. Lane‑change decision (unchanged, but uses new kinematics) ─
				const float dist_to_node = rt.length - s_f;
				const AgentData& ag = agents[i];

				// Current and (first) desired lane indices on this edge
				const int curr_idx = rt.static_lane->index_in_edge; // 0 = right-most
				int goal_idx = -1;
				for (int b = 0; b < 32; ++b) { // first set bit in goal mask
					if ((ag.goal_lane_mask >> b) & 1) {
						goal_idx = b;
						break;
					}
				}

				if (goal_idx >= 0 && goal_idx != curr_idx && !VehicleStateBitsV::has_info(buf.flags[i], VehicleStateBits::FL_COOLDOWN)) {
					const int lanes_delta = goal_idx - curr_idx; // +ve ⇒ need to go LEFT
					const float prep = D_PREP + std::abs(lanes_delta) * D_PREP_PER_LANE;

					if (dist_to_node < prep) {
						Lane* neigh = (lanes_delta > 0) ? rt.static_lane->left() // move leftwards
														  :
														  rt.static_lane->right(); // move rightwards

						if (neigh) {
							buf.lane_target[i] = neigh; // step one lane toward goal
							VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::ST_EXECUTE, VehicleStateBitsDivision::STATE);
						}
					}
				}

				// ─── 5. Cool‑down bookkeeping (unchanged) ────────────────────────
				if (VehicleStateBitsV::has_info(buf.flags[i], VehicleStateBits::FL_COOLDOWN)) {
					float t = buf.lateral_off[i];
					t += static_cast<float>(dt);
					if (t > T_MIN) {
						VehicleStateBitsV::remove_info(buf.flags[i], VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);
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
	inline Lane* choose_entry_lane(const Lane* src_lane, const Edge* next_edge, VehicleMovementErrors& err) {
		Lane* best = nullptr;
		bool best_is_yield = true; // so non-yield wins
		int best_shift = INT_MAX;  // minimise |Δlane|

		const std::size_t src_idx = src_lane->index_in_edge; // local index in its edge
		err = VehicleMovementErrors::ER_NO_ERROR;
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
			err = VehicleMovementErrors::ER_NO_OUTGOING_CONNECTION;
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
			err = has_connection ? VehicleMovementErrors::ER_INCORRECT_LANE : VehicleMovementErrors::ER_INCORRECT_EDGE;
			// TODO[simulation]: algo error handling
			//throw std::runtime_error(
			//	"Route impossible: no LaneLink from edge " + std::to_string(src_lane->parent->get_id()) + " to edge " + std::to_string(next_edge->get_id()) + '.');
			return src_lane->outgoing_connections[0]->to;
		}
		err = VehicleMovementErrors::ER_NO_ERROR;
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
			const auto it = std::find(v_src.begin(), v_src.end(), row);
			if (it != v_src.end()) {
				// Swap‑and‑pop for O(1) physical erase …
				std::size_t moved = v_src.back();
				*it = moved;
				v_src.pop_back();

				// … but now the element that was at the tail sits at *it* and may
				// violate descending order.  Re‑insert it where it belongs.
				if (!v_src.empty()) {
					auto correct = std::upper_bound(
						v_src.begin(), v_src.end(), moved,
						[&](std::size_t lhs, std::size_t rhs) {
							return s_curr[lhs] > s_curr[rhs]; // descending
						});

					// Rotate [first, middle, last): moves *it* to `correct` with O(distance)
					if (correct < it) {
						std::rotate(correct, it, it + 1);
					} else if (correct > it) {
						std::rotate(it, it + 1, correct);
					}
				}
			}
		}

		/* ---- 2. O(log N) insert into target, keep sorted -------------- */
		const double s = s_curr[row];
		auto it_ins = std::lower_bound(
			v_tgt.begin(), v_tgt.end(), s,
			[&](std::size_t j, double pos) {
				return s_curr[j] > pos;
			});

		v_tgt.insert(it_ins, row);
	}

	inline bool gap_ok(const LaneRuntime& tgt_rt,
		const std::vector<double>& s_curr,
		const std::vector<float>& length,
		const std::vector<float>& v_curr,
		const double s_new, // tentative bumper pos
		const float len_new,
		const sim::idm_params_t& p,
		const double dt,
		const std::size_t row_newcomer) {
		const auto& idx = tgt_rt.idx; // descending s_curr

		// Find insertion point (same as before)
		auto it = std::lower_bound(idx.begin(), idx.end(), s_new,
			[&](std::size_t j, double pos) {
				return s_curr[j] > pos;
			});

		auto enough_gap_and_brake = [&](float gap,
										float v_follow,
										float v_lead) -> bool {
			if (gap < p.s0) {
				return false; // hard minimum jam distance
			}

			float delta_v = v_follow - v_lead; // +ve when closing
			if (delta_v <= 0.0f) {
				return true; // diverging – fine
			}

			// (i) Brake needed to **stop** before leader using constant decel
			float req_brake = (delta_v * delta_v) / (2.0f * std::max(1e-3f, gap - p.s0));
			if (req_brake > p.b_hard + 1e-4f) {
				return false;
			}

			// (ii) Per‑tick decel limit (so we don't exceed −b_comf in this step)
			float per_tick = delta_v / static_cast<float>(dt);
			if (per_tick > p.b_hard + 1e-4f) {
				// return false;
			}

			return true;
		};

		/* ---------- leader gap ------------------------------------------------- */
		if (it != idx.begin()) {
			std::size_t j_lead = *(it - 1);
			float gap = sim::actual_gap(static_cast<float>(s_curr[j_lead]),
				static_cast<float>(s_new),
				length[j_lead], len_new);
			if (!enough_gap_and_brake(gap, /* follower = newcomer */
					v_curr[row_newcomer], v_curr[j_lead])) {
				return false;
			}
		}

		/* ---------- follower (vehicle behind newcomer) ------------------------- */
		if (it != idx.end()) {
			std::size_t j_follow = *it;
			float gap = sim::actual_gap(static_cast<float>(s_new),
				static_cast<float>(s_curr[j_follow]),
				len_new, length[j_follow]);
			if (!enough_gap_and_brake(gap, /* follower behind */
					v_curr[j_follow], v_curr[row_newcomer])) {
				return false;
			}
		}

		return true;
	}

	void make_stop() {
	}

	void phase2_commit(
		TrafficSimulationSystem& system,
		VehicleBuffers& buf,
		std::vector<LaneRuntime>& lane_rt,
		const double dt) // unchanged param list
	{
		TJS_TRACY_NAMED("VehicleMovement::IDM::Phase2");

#if TJS_SIMULATION_DEBUG
		auto& debug = system.settings().debug_data;
#endif

		auto& agents = system.agents();

		buf.s_curr.swap(buf.s_next);
		buf.v_curr.swap(buf.v_next);

		static const sim::idm_params_t p_idm {};

		/* ---------------- lateral loop --------------------------------------- */
		for (LaneRuntime& rt : lane_rt) {
			auto idx_copy = rt.idx;
			for (std::size_t row : idx_copy) {
				Lane* tgt = buf.lane_target[row];
				if (!tgt) {
					continue;
				}

				if (VehicleStateBitsV::has_info(buf.flags[row], VehicleStateBits::ST_STOPPED)) {
					buf.lane_target[row] = nullptr;
					continue;
				}

				if (gap_ok(lane_rt[tgt->index_in_buffer],
						buf.s_curr, buf.length, buf.v_curr,
						buf.s_curr[row], buf.length[row],
						p_idm, dt, row)) {
					move_index(row, lane_rt, rt.static_lane, tgt, buf.s_curr);
					buf.lane[row] = tgt;
					buf.lane_target[row] = nullptr;
					VehicleStateBitsV::set_info(buf.flags[row], VehicleStateBits::ST_FOLLOW, VehicleStateBitsDivision::STATE);
					VehicleStateBitsV::set_info(buf.flags[row], VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);

					const auto& tgt_idx = lane_rt[tgt->index_in_buffer].idx;
					if (!tgt_idx.empty() && tgt_idx.front() != row) {
						std::size_t j_lead = tgt_idx.front();
						float gap_leader = sim::actual_gap(static_cast<float>(buf.s_curr[j_lead]),
							static_cast<float>(buf.s_curr[row]),
							buf.length[j_lead], buf.length[row]);
						float v_safe = sim::safe_entry_speed(buf.v_curr[j_lead], gap_leader, dt);
						buf.v_curr[row] = std::clamp(v_safe, 0.0f, buf.v_curr[row]);
					}
				}
			}
		}

		/* ---------------- edge hop loop -------------------------------------- */
		for (std::size_t i = 0; i < agents.size(); ++i) {
			AgentData& ag = agents[i];
			if (ag.path.empty()) {
				continue;
			}

			double remain = buf.s_curr[i];
			Lane* lane = buf.lane[i];

			TJS_BREAK_IF(
				debug.movement_phase == SimulationMovementPhase::IDM_Phase2_Agent
				&& i == debug.agent_id
				&& debug.lane_id == lane->get_id()
				&& debug.vehicle_indices == lane_rt[lane->index_in_buffer].idx);

			while (remain >= lane->length - 1e-6) {
				remain -= lane->length;

				++ag.path_offset;
				if (ag.path_offset >= ag.path.size()) {
					//move_index(i, lane_rt, lane, lane, buf.s_curr);
					VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
					VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);

					ag.vehicle->error = VehicleMovementErrors::ER_NO_PATH;
					buf.s_curr[i] = lane->length;
					buf.s_next[i] = buf.s_curr[i];
					break;
				}

				Edge* next_edge = ag.path[ag.path_offset];
				VehicleMovementErrors err;

				TJS_BREAK_IF(
					debug.movement_phase == SimulationMovementPhase::IDM_Phase2_ChooseLane
					&& i == debug.agent_id
					&& debug.lane_id == lane->get_id()
					&& debug.vehicle_indices == lane_rt[lane->index_in_buffer].idx);

				Lane* entry = choose_entry_lane(lane, next_edge, err);
				if (err != VehicleMovementErrors::ER_NO_ERROR || !entry) {
					VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
					VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);

					ag.vehicle->error = err;
					buf.s_curr[i] = lane->length - 0.01;
					buf.s_next[i] = buf.s_curr[i];
					buf.v_curr[i] = 0.0f;
					buf.v_next[i] = 0.0f;
					break;
				}

				if (!gap_ok(lane_rt[entry->index_in_buffer],
						buf.s_curr, buf.length, buf.v_curr,
						remain, buf.length[i],
						p_idm, dt, i)) {
					buf.s_curr[i] = lane->length - 0.01;
					buf.s_next[i] = buf.s_curr[i];
					break;
				}

				/* ----- commit hop ------------------------------------------- */
				buf.s_curr[i] = remain;
				move_index(i, lane_rt, lane, entry, buf.s_curr);
				lane = entry;
				buf.lane[i] = entry;
				ag.goal_lane_mask = build_goal_mask(*entry->parent, *ag.path[ag.path_offset + 1]);

				/* ----- SUMO‑style speed clamp ------------------------------ */
				const auto& tgt_idx = lane_rt[entry->index_in_buffer].idx;
				if (!tgt_idx.empty() && tgt_idx.front() != i) {
					std::size_t j_lead = tgt_idx.front();
					float gap_leader = sim::actual_gap(static_cast<float>(buf.s_curr[j_lead]),
						static_cast<float>(buf.s_curr[i]),
						buf.length[j_lead], buf.length[i]);
					float v_safe = sim::safe_entry_speed(buf.v_curr[j_lead], gap_leader, dt);
					buf.v_curr[i] = std::clamp(v_safe, 0.0f, buf.v_curr[i]);
				}
			}
		}
	}

} // namespace tjs::core::simulation
