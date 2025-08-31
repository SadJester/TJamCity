#include <core/stdafx.h>

#include <core/simulation/movement/idm/lane_agnostic_movement.h>
#include <core/simulation/movement/idm/idm_utils.h>

#include <core/simulation/movement/movement_utils.h>

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/math_constants.h>
#include <core/map_math/earth_math.h>

#include <core/data_layer/lane.h>
#include <core/data_layer/edge.h>
#include <core/data_layer/vehicle.h>

#include <core/simulation/agent/agent_data.h>
#include <core/simulation/simulation_system.h>

namespace tjs::core::simulation {
	namespace idm {

		float idm_scalar(const float v_follower,
			const float v_desired,
			const float s_gap,
			const idm_params_t& p) noexcept {
			const float delta_v = v_follower - v_desired; // closing speed
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

		void move_index(Vehicle* vehicle_ptr,
			std::vector<LaneRuntime>& lane_rt,
			const Lane* src,
			const Lane* tgt,
			bool shadow) {
			// nothing to do
			if (src == tgt) {
				return;
			}

			auto& v_src = lane_rt[src->index_in_buffer].idx;
			auto& v_tgt = lane_rt[tgt->index_in_buffer].idx;

			/* ---- 1. O(1) erase from source by swap-and-pop ---------------- */
			if (!shadow) {
				const auto it = std::find(v_src.begin(), v_src.end(), vehicle_ptr);
				if (it != v_src.end()) {
					Vehicle* moved = v_src.back();
					// it's not the last element
					const bool need_reinsert = !v_src.empty() && (it != v_src.end() - 1);

					*it = moved;
					v_src.pop_back();

					// … but now the element that was at the tail sits at *it* and may
					// violate descending order.  Re‑insert it where it belongs.
					if (!v_src.empty() && need_reinsert) {
						auto correct = std::upper_bound(
							v_src.begin(), v_src.end(), moved,
							[&](Vehicle* lhs, Vehicle* rhs) {
								return lhs->s_on_lane > rhs->s_on_lane; // descending
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
			if (std::ranges::find(v_tgt, vehicle_ptr) == v_tgt.end()) {
				const double s = vehicle_ptr->s_on_lane;
				auto it_ins = std::lower_bound(
					v_tgt.begin(), v_tgt.end(), s,
					[&](Vehicle* j, double pos) {
						return j->s_on_lane > pos;
					});

				v_tgt.insert(it_ins, vehicle_ptr);
			}
		}

		Lane* choose_entry_lane(const Lane* src_lane, const Edge* next_edge, VehicleMovementError& err) {
			Lane* best = nullptr;
			bool best_is_yield = true; // so non-yield wins
			int best_shift = INT_MAX;  // minimise |Δlane|

			const std::size_t src_idx = src_lane->index_in_edge; // local index in its edge
			err = VehicleMovementError::ER_NO_ERROR;
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
				err = VehicleMovementError::ER_NO_OUTGOING_CONNECTION;
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
				err = has_connection ? VehicleMovementError::ER_INCORRECT_LANE : VehicleMovementError::ER_INCORRECT_EDGE;
				// TODO[simulation]: algo error handling
				//throw std::runtime_error(
				//	"Route impossible: no LaneLink from edge " + std::to_string(src_lane->parent->get_id()) + " to edge " + std::to_string(next_edge->get_id()) + '.');
				return src_lane->outgoing_connections[0]->to;
			}
			err = VehicleMovementError::ER_NO_ERROR;
			return best;
		}

		inline Vehicle* tgt_leader(const std::vector<Vehicle*>& idx, const Vehicle& self_v) {
			auto it = std::lower_bound(idx.begin(), idx.end(), self_v.s_on_lane,
				[&self_v](const Vehicle* v, double pos) {
					return v->s_on_lane > pos;
				});
			return (it != idx.begin()) ? *(it - 1) : nullptr;
		}
		inline Vehicle* tgt_follower(const std::vector<Vehicle*>& idx, const Vehicle& self_v) {
			auto it = std::lower_bound(idx.begin(), idx.end(), self_v.s_on_lane,
				[](const Vehicle* v, double pos) {
					return v->s_on_lane > pos;
				});

			if (it == idx.end()) {
				return nullptr;
			}

			if (*it == &self_v) {
				++it;
			}

			return (it != idx.end()) ? *it : nullptr;
		}

		// Returns nearest set bit to curr_idx within [0, lanes_count).
		// If the current bit is set → returns curr_idx. If mask==0 → returns -1.
		inline int nearest_goal_idx(uint32_t mask, int curr_idx, int lanes_count) noexcept {
			if (lanes_count <= 0) {
				return -1;
			}

			// clamp mask to existing lanes (avoid bits beyond lane count)
			if (lanes_count < 32) {
				uint32_t low_bits = (lanes_count == 32) ? 0xFFFFFFFFu : ((1u << lanes_count) - 1u);
				mask &= low_bits;
			}

			if (mask == 0u) {
				return -1; // no allowed lanes at all
			}
			if (curr_idx >= 0 && curr_idx < lanes_count) {
				if (mask & (1u << curr_idx)) {
					return curr_idx; // already allowed
				}
			} else {
				// out-of-range current index: clamp for search symmetry
				curr_idx = std::clamp(curr_idx, 0, lanes_count - 1);
			}

			// search symmetrically: distance 1, then 2, ...
			for (int d = 1; d < lanes_count; ++d) {
				int left = curr_idx - d;
				int right = curr_idx + d;
				if (left >= 0 && (mask & (1u << left))) {
					return left;
				}
				if (right < lanes_count && (mask & (1u << right))) {
					return right;
				}
			}
			return -1; // should not happen if mask!=0 and lanes_count>0
		}

		void phase1_simd(
			TrafficSimulationSystem& system,
			const std::vector<AgentData*>& agents,
			const std::vector<LaneRuntime>& lane_rt,
			const double dt) {
			TJS_TRACY_NAMED("VehicleMovement::IDM::Phase1");

			static constexpr float D_PREP_PER_LANE = 60.0f; // extra [m] per missing lane
			static constexpr float D_PREP = 80.0f;          // distance to start lane‑prep [m]
			static constexpr float T_MIN = 1.5f;            // cool‑down expiry threshold [s]

#if TJS_SIMULATION_DEBUG
			auto& debug = system.settings().debug_data;
#endif

			idm::idm_params_t idm_def {}; // default calibrated parameters

			/* threaded outer loop over lanes (keep free to add OpenMP/TBB) */
			// #pragma omp parallel for schedule(dynamic,4)
			for (std::size_t L = 0; L < lane_rt.size(); ++L) {
				const LaneRuntime& rt = lane_rt[L];
				const auto& idx = rt.idx; // sorted rear→front vehicle pointers
				const std::size_t n = idx.size();

				TJS_BREAK_IF(
					debug.movement_phase == SimulationMovementPhase::IDM_Phase1_Lane
					&& debug.lane_id == rt.static_lane->get_id());

				// ---------------------------------------------------------------------
				// Scalar inner loop – one follower row at a time (will become gather/SIMD)
				// ---------------------------------------------------------------------
				for (std::size_t k = 0; k < n; ++k) {
					Vehicle* vehicle = idx[k]; // follower vehicle pointer

					TJS_BREAK_IF(
						debug.movement_phase == SimulationMovementPhase::IDM_Phase1_Vehicle
						&& debug.lane_id == rt.static_lane->get_id()
						&& debug.vehicle_indices.size() == 1
						&& k == debug.vehicle_indices[0]);

					// Skip broken cars
					if (VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::FL_ERROR)) {
						continue;
					}

					if (vehicle->is_merging(*rt.static_lane)) {
						continue;
					}

					// ─── 1. Gather follower state ────────────────────────────────────
					const float s_f = static_cast<float>(vehicle->s_on_lane); // [m]
					const float v_f = vehicle->currentSpeed;                  // [m/s]
					const float l_f = vehicle->length;                        // bumper‑to‑bumper length [m]

					// ─── 1b. Gather leader state (if any) ────────────────────────────
					float s_gap = 1e9f;   // sentinel = "free road"
					float v_leader = v_f; // same speed → Δv = 0

					if (debug.agent_id == vehicle->uid) {
						std::cout << "";
					}

					// Find leader in current lane (or check it in coherent lane)
					int candidate = k - 1;
					while (candidate >= 0) {
						const Vehicle* leader = idx[candidate];
						--candidate;

						v_leader = leader->currentSpeed;
						s_gap = idm::actual_gap(leader->s_on_lane, s_f, leader->length, leader->length);
						break;
					}

					// try to pass vehicle
					float a_cooperative = std::numeric_limits<float>::max();
					if (idm_def.is_cooperating
						&& VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::ST_FOLLOW)
						&& !VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::FL_COOLDOWN)) {
						if (vehicle->cooperation_vehicle == nullptr) {
							for (auto it_slot = rt.vehicle_slots.rbegin(); it_slot != rt.vehicle_slots.rend(); ++it_slot) {
								float gg = (*it_slot)->s_on_lane - s_f;
								if (gg > 15.0f) {
									break;
								}
								if (gg > 0.0f) {
									vehicle->cooperation_vehicle = *it_slot;
									vehicle->action_time = 0.0;
									break;
								}
							}
						}
					} else {
						vehicle->cooperation_vehicle = nullptr;
					}

					if (vehicle->cooperation_vehicle != nullptr) {
						// reset cooperation vehicle if it is not merging in our lane or not in prepare state
						if (!VehicleStateBitsV::has_info(vehicle->cooperation_vehicle->state, VehicleStateBits::ST_PREPARE) || vehicle->cooperation_vehicle->lane_target != rt.static_lane) {
							vehicle->cooperation_vehicle = nullptr;
						} else {
							vehicle->action_time += dt;
							if (vehicle->action_time >= idm_def.t_max_coop_time) {
								vehicle->cooperation_vehicle = nullptr;
								vehicle->action_time = 0.0;
								VehicleStateBitsV::set_info(vehicle->state, VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);
							} else {
								float merging_gap = idm::actual_gap(
									vehicle->cooperation_vehicle->s_on_lane, s_f, vehicle->cooperation_vehicle->length, vehicle->cooperation_vehicle->length);
								a_cooperative = idm::idm_scalar(v_f, vehicle->cooperation_vehicle->currentSpeed, merging_gap, idm_def);
								a_cooperative = std::max(a_cooperative, -idm_def.a_coop_max);
							}
						}
					}

					// check for leader in target lane
					if (
						VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::ST_CROSS)
						&& vehicle->lane_target != nullptr) {
						const auto& tgt_rt = lane_rt[vehicle->lane_target->index_in_buffer];
						auto tgt_lead = tgt_leader(tgt_rt.idx, *vehicle);
						if (tgt_lead != nullptr) {
							v_leader = std::min(v_leader, tgt_lead->currentSpeed);

							s_gap = std::min(
								s_gap,
								idm::actual_gap(tgt_lead->s_on_lane, s_f, tgt_lead->length, tgt_lead->length));
						}
					}

					// ─── 2. IDM acceleration ────────────────────────────────────────
					const float a_self = idm::idm_scalar(v_f, v_leader, s_gap, idm_def);
					const float a = std::max(-idm_def.b_hard, std::min(a_self, a_cooperative)); // clamp by hard brake

					// ─── 3. Kinematics update (Euler forward) ────────────────────────
					const float v_next = std::clamp(v_f + a * static_cast<float>(dt), 0.0f, rt.max_speed + idm_def.v_limits_violate);
					vehicle->v_next = v_next;
					vehicle->s_next = s_f + v_f * dt + 0.5f * a * static_cast<float>(dt * dt);

					if (debug.agent_id == vehicle->uid && vehicle->v_next < vehicle->currentSpeed) {
						std::cout << "";
					}

					// ─── 4. Lane‑change decision (unchanged, but uses new kinematics) ─
					const float dist_to_node = rt.length - s_f;

					const uint16_t change_state = static_cast<int>(VehicleStateBits::ST_PREPARE) | static_cast<int>(VehicleStateBits::ST_CROSS) | static_cast<int>(VehicleStateBits::ST_ALIGN);
					if (!VehicleStateBitsV::has_any(vehicle->state, change_state, VehicleStateBitsDivision::STATE)) {
						const int curr_idx = rt.static_lane->index_in_edge; // 0 = right-most
						const int lanes_cnt = static_cast<int>(rt.static_lane->parent->lanes.size());
						const uint32_t mask = vehicle->goal_lane_mask;
						const int goal_idx = nearest_goal_idx(mask, curr_idx, lanes_cnt);

						if (goal_idx >= 0 && goal_idx != curr_idx && !VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::FL_COOLDOWN)) {
							const int lanes_delta = goal_idx - curr_idx; // +ve ⇒ need to go LEFT
							// preparation for one lane and extra 0.8 for each lane above 1
							const float prep = idm_def.s_preparation + (std::abs(lanes_delta) - 1) * idm_def.s_preparation * 0.8f;

							if (dist_to_node < prep) {
								Lane* neigh = (lanes_delta > 0) ? rt.static_lane->left() : rt.static_lane->right();
								if (neigh) {
									if (vehicle->current_lane->get_id() == 28 && neigh->get_id() == 5) {
										std::cout << "";
									}
									vehicle->lane_target = neigh; // step one lane toward goal
									VehicleStateBitsV::set_info(vehicle->state, VehicleStateBits::ST_PREPARE, VehicleStateBitsDivision::STATE);
									vehicle->action_time = 0.0;
								}
							}
						}
					}

					// ─── 5. Cool‑down bookkeeping (unchanged) ────────────────────────
					if (VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::FL_COOLDOWN)) {
						vehicle->action_time += dt;
						if (vehicle->action_time > idm_def.t_cooldown) {
							VehicleStateBitsV::remove_info(vehicle->state, VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);
							vehicle->action_time = 0.0f;
						}
					}
				}
			}
		}

		bool check_can_switch(
			const std::vector<LaneRuntime>& lane_rt,
			const Lane* tgt,
			Vehicle* vehicle,
			const idm_params_t& p_idm,
			bool is_mandatory_switch) {
			// --- current-lane leader (for a_old) ---
			const auto& idx_curr = lane_rt[vehicle->current_lane->index_in_buffer].idx;
			Vehicle* curr_lead = nullptr;
			if (idx_curr.size() > 1) {
				auto it = std::find(idx_curr.begin(), idx_curr.end(), vehicle);
				if (it != idx_curr.begin()) {
					curr_lead = *(it - 1);
				}
			}
			float gap_curr = std::numeric_limits<float>::infinity();
			float v_lead_curr = vehicle->currentSpeed;
			if (curr_lead) {
				gap_curr = idm::actual_gap((float)curr_lead->s_on_lane,
					(float)vehicle->s_on_lane,
					curr_lead->length, vehicle->length);
				v_lead_curr = curr_lead->currentSpeed;
			}

			// --- target-lane neighbors ---
			const auto& idx_tgt = lane_rt[tgt->index_in_buffer].idx;
			Vehicle* lead = tgt_leader(idx_tgt, *vehicle);
			Vehicle* foll = tgt_follower(idx_tgt, *vehicle);

			// front gap (ego vs target-lane leader)
			float gap_front = std::numeric_limits<float>::infinity();
			float v_lead = vehicle->currentSpeed;
			if (lead) {
				gap_front = idm::actual_gap((float)lead->s_on_lane,
					(float)vehicle->s_on_lane,
					lead->length, vehicle->length);
				v_lead = lead->currentSpeed;
			}

			// divide t_headway by coeff to change faster
			const float req_gap = std::max(p_idm.t_headway * p_idm.t_cross_headway_coeff * vehicle->currentSpeed, p_idm.s0);
			const bool front_ok = (gap_front >= req_gap);

			// rear safety (new follower vs ego as new leader)
			bool rear_safe = true;
			if (foll) {
				const float gap_rear = idm::actual_gap((float)vehicle->s_on_lane,
					(float)foll->s_on_lane,
					vehicle->length, foll->length);
				const float a_after = idm::idm_scalar(foll->currentSpeed,
					vehicle->currentSpeed,
					gap_rear, p_idm);
				rear_safe = (a_after >= -p_idm.b_comf);
			}

			// --- Benefit / politeness (keep your mandatory override) ---
			const float a_old = idm::idm_scalar(vehicle->currentSpeed, v_lead_curr, gap_curr, p_idm);
			const float a_new = idm::idm_scalar(vehicle->currentSpeed, v_lead, gap_front, p_idm);
			const float benefit = a_new - a_old;
			const bool polite = (is_mandatory_switch && a_new > -(p_idm.b_comf * 2.0f)) || (benefit > p_idm.a_politeness_threshold);

			const bool ok = front_ok && rear_safe && polite;
			return ok;
		}

		void flush_target(Vehicle* v, std::vector<LaneRuntime>& lane_rt) {
			if (v->lane_target == nullptr) {
				return;
			}
			auto& rt = lane_rt[v->lane_target->index_in_buffer];
			auto it = std::ranges::find(rt.idx, v);
			if (it != rt.idx.end()) {
				rt.idx.erase(it);
			}

			auto it_slot = std::ranges::find(rt.vehicle_slots, v);
			if (it_slot != rt.vehicle_slots.end()) {
				rt.vehicle_slots.erase(it_slot);
			}

			v->lane_target = nullptr;
		}

		void phase2_commit(
			TrafficSimulationSystem& system,
			const std::vector<AgentData*>& agents,
			std::vector<LaneRuntime>& lane_rt,
			const double dt) // unchanged param list
		{
			TJS_TRACY_NAMED("VehicleMovement::IDM::Phase2");

#if TJS_SIMULATION_DEBUG
			auto& debug = system.settings().debug_data;
#endif

			// Swap current and next values for all vehicles
			for (auto agent : agents) {
				auto& vehicle = *agent->vehicle;
				vehicle.has_position_changes = vehicle.s_on_lane != vehicle.s_next;
				vehicle.s_on_lane = vehicle.s_next;
				vehicle.currentSpeed = vehicle.v_next;
			}

			static const idm::idm_params_t p_idm {};

			// Extra structure so first cycle could be constant with indices
			struct PendingMove {
				Vehicle* vehicle;
				Lane* src;
				Lane* tgt;
				bool shadow;
			};

			std::vector<PendingMove> pending_moves;
			// Suppose that 10% will be moved in one tick
			pending_moves.reserve(agents.size() / 10);

			/*auto _check = [&lane_rt](const Lane& _lane_s) {
				auto _lane = lane_rt[_lane_s.index_in_buffer];
				float s = std::numeric_limits<float>::max();
				for (size_t i = 0; i < _lane.idx.size(); ++i) {
					auto v = _lane.idx[i];
					if (VehicleStateBitsV::has_info(v->state, VehicleStateBits::ST_PREPARE)) {
						continue;
					}
					if (v->s_on_lane > s) {
						return (int)i;
					}
					s = v->s_on_lane;
				}
				return -1;
			};*/

			/* ---------------- lateral loop --------------------------------------- */
			for (const LaneRuntime& rt : lane_rt) {
				for (Vehicle* vehicle : rt.idx) {
					const bool is_shadow = rt.static_lane != vehicle->current_lane && vehicle->lane_target != nullptr;
					if (is_shadow) {
						continue;
					}

					if (VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::ST_STOPPED)) {
						continue;
					}

					Lane* tgt = vehicle->lane_target;
					if (VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::ST_PREPARE) && tgt) {
						vehicle->action_time += dt;
						bool ready = vehicle->action_time >= p_idm.t_prepare;

						if (debug.agent_id == vehicle->uid) {
							std::cout << "";
						}

						// insert shadow
						if (auto& rt_tgt = lane_rt[tgt->index_in_buffer]; std::ranges::find(rt_tgt.vehicle_slots, vehicle) == rt_tgt.vehicle_slots.end()) {
							auto it_ins = std::lower_bound(rt_tgt.vehicle_slots.begin(), rt_tgt.vehicle_slots.end(),
								vehicle->s_on_lane, [](Vehicle* v, float s) {
									return v->s_on_lane > s;
								});
							rt_tgt.vehicle_slots.insert(it_ins, vehicle);
						}

						bool can_switch = ready && check_can_switch(lane_rt, tgt, vehicle, p_idm, true);
						if (ready && can_switch) {
							if (debug.agent_id == vehicle->uid) {
								std::cout << "";
							}
							vehicle->has_position_changes = true;

							auto& start = vehicle->current_lane->centerLine.front();
							auto& end = vehicle->current_lane->centerLine.back();

							const bool positive_dir = algo::is_in_first_or_fourth(start, end, start, tgt->centerLine.front());
							vehicle->lane_change_dir = positive_dir ? 1 : -1;
							vehicle->lateral_offset = 0.0f;
							vehicle->action_time = 0.0;
							VehicleStateBitsV::overwrite_info(vehicle->state, VehicleStateBits::ST_CROSS, VehicleStateBitsDivision::STATE);
							continue;
						}
					} else if (VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::ST_CROSS)) {
						if (debug.agent_id == vehicle->uid) {
							std::cout << "";
						}
						vehicle->action_time += dt;
						double prog = std::min(vehicle->action_time / p_idm.t_cross, 1.0);
						double cos_term = std::sin(tjs::core::MathConstants::M_PI * 0.5 * prog);
						vehicle->lateral_offset = vehicle->lane_change_dir * vehicle->current_lane->width * cos_term;
						vehicle->has_position_changes = true;
						if (prog >= 1.0f) {
							if (debug.agent_id == vehicle->uid) {
								std::cout << "";
							}
							pending_moves.push_back(PendingMove { vehicle, rt.static_lane, tgt, false });
							flush_target(vehicle, lane_rt);

							vehicle->lateral_offset = 0.0f;
							vehicle->action_time = 0.0;
							VehicleStateBitsV::overwrite_info(vehicle->state, VehicleStateBits::ST_ALIGN, VehicleStateBitsDivision::STATE);
						}
					} else if (VehicleStateBitsV::has_info(vehicle->state, VehicleStateBits::ST_ALIGN)) {
						vehicle->action_time += dt;
						if (vehicle->action_time >= p_idm.t_align) {
							VehicleStateBitsV::overwrite_info(vehicle->state, VehicleStateBits::ST_FOLLOW, VehicleStateBitsDivision::STATE);
							VehicleStateBitsV::set_info(vehicle->state, VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);
							vehicle->action_time = 0.0;
						}
					}
				}
			}

			/* ----------------  Do all moves after scanning--------------------------------------- */
			for (const auto& m : pending_moves) {
				idm::move_index(m.vehicle, lane_rt, m.src, m.tgt, m.shadow);
				if (!m.shadow) {
					m.vehicle->current_lane = m.tgt;
					if (m.vehicle->lane_target != nullptr) {
						std::cout << "";
					}
				}
			}

			using VSB = VehicleStateBits;
			using DIV = VehicleStateBitsDivision;
			using bit_t = uint16_t;

			/* A small helper to test whether a car is still measurably off‑centre. */
			const auto off_centre = [](Vehicle& vehicle) noexcept {
				return std::fabs(vehicle.lateral_offset) > 0.5f; // 50 cm tolerance
			};

			/* ---------------- edge hop loop -------------------------------------- */
			for (std::size_t i = 0; i < agents.size(); ++i) {
				AgentData& ag = *agents[i];
				if (ag.path.empty()) {
					continue;
				}

				auto& v = *ag.vehicle;

				double remain = v.s_on_lane;
				Lane* lane = v.current_lane;

				TJS_BREAK_IF(
					debug.movement_phase == SimulationMovementPhase::IDM_Phase2_Agent
					&& i == debug.agent_id
					&& debug.lane_id == lane->get_id());

				while (remain >= lane->length - 1e-6) {
					if (v.uid == debug.agent_id) {
						std::cout << "";
					}
					remain -= lane->length;

					++ag.path_offset;
					if (ag.path_offset >= ag.path.size()) {
						flush_target(&v, lane_rt);
						stop_moving(i, ag, v, lane, VehicleMovementError::ER_NO_PATH);
						break;
					}

					Edge* next_edge = ag.path[ag.path_offset];
					VehicleMovementError err;

					TJS_BREAK_IF(
						debug.movement_phase == SimulationMovementPhase::IDM_Phase2_ChooseLane
						&& i == debug.agent_id
						&& debug.lane_id == lane->get_id());

					Lane* entry = choose_entry_lane(lane, next_edge, err);
					if (err != VehicleMovementError::ER_NO_ERROR || !entry) {
						flush_target(&v, lane_rt);
						stop_moving(i, ag, v, lane, err);
						break;
					}

					if (!gap_ok(lane_rt[entry->index_in_buffer],
							v.currentSpeed,
							remain, v.length,
							p_idm, dt)) {
						v.s_on_lane = lane->length - 0.01;
						v.s_next = v.s_on_lane;
						break;
					}

					/* -----------------------------------------------
					* Avoid node transitions while a lateral
					* manoeuvre (prepare / cross / align) is active.
					* --------------------------------------------- */
					// Changing lane in this cycle has more priority over changing lane in lateral movement because
					bit_t change_mask = (bit_t)VSB::ST_PREPARE | (bit_t)VSB::ST_CROSS;
					bool busy = VehicleStateBitsV::has_any(v.state, change_mask, DIV::STATE) || (VehicleStateBitsV::has_info(v.state, VSB::ST_ALIGN) && off_centre(v));
					if (busy) {
						if (remain >= lane->length - 1e-3) {
							v.s_on_lane = v.s_next = lane->length - 1e-3;
						}
						break;
					}

					/* ----- commit hop ------------------------------------------- */
					v.s_on_lane = remain;
					idm::move_index(&v, lane_rt, lane, entry);
					flush_target(&v, lane_rt);

					lane = entry;
					v.current_lane = entry;

					if (ag.path_offset < ag.path.size() - 1) {
						v.goal_lane_mask = build_goal_mask(*entry->parent, *ag.path[ag.path_offset + 1]);
					} else {
						v.goal_lane_mask = 0xFFFF;
					}
					/* ----- SUMO‑style speed clamp ------------------------------ */
					const auto& tgt_idx = lane_rt[entry->index_in_buffer].idx;
					if (!tgt_idx.empty() && tgt_idx.front() != &v) {
						Vehicle* j_lead = tgt_idx.front();
						float gap_leader = idm::actual_gap(static_cast<float>(j_lead->s_on_lane),
							static_cast<float>(v.s_on_lane),
							j_lead->length, v.length);
						float v_safe = idm::safe_entry_speed(j_lead->currentSpeed, gap_leader, dt);
						v.currentSpeed = std::clamp(v_safe, 0.0f, v.currentSpeed);
					}
				}
			}
		}
	} // namespace idm
} // namespace tjs::core::simulation
