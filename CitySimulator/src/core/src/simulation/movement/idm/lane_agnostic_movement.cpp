#include <core/stdafx.h>

#include <core/simulation/movement/idm/lane_agnostic_movement.h>
#include <core/simulation/movement/idm/idm_utils.h>

#include <core/simulation/movement/movement_utils.h>

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/math_constants.h>

#include <core/data_layer/lane.h>
#include <core/data_layer/edge.h>
#include <core/data_layer/vehicle.h>

#include <core/simulation/agent/agent_data.h>
#include <core/simulation/simulation_system.h>

namespace {
	constexpr float T_PREPARE = 0.1f;
	constexpr float T_CROSS = 2.0f;
	constexpr float T_ALIGN = 0.1f;
	constexpr float POLITENESS_THRESHOLD = 0.2f;
	constexpr float TAU = 1.0f;
	constexpr float DELTA = 1.0f;
	constexpr float MIN_GAP = 2.0f;

	static int VEHICLE_ID = 59;
} // namespace

namespace tjs::core::simulation {
	namespace idm {

		float idm_scalar(const float v_follower,
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

		void move_index(std::size_t row,
			std::vector<LaneRuntime>& lane_rt,
			const Lane* src,
			const Lane* tgt,
			const std::vector<Vehicle>& vehicles) {
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
					std::size_t moved = v_src.back();
					// it's not the last element
					const bool need_reinsert = !v_src.empty() && (it != v_src.end() - 1);

					*it = moved;
					v_src.pop_back();

					// … but now the element that was at the tail sits at *it* and may
					// violate descending order.  Re‑insert it where it belongs.
					if (!v_src.empty() && need_reinsert) {
						auto correct = std::upper_bound(
							v_src.begin(), v_src.end(), moved,
							[&](std::size_t lhs, std::size_t rhs) {
								return vehicles[lhs].s_on_lane > vehicles[rhs].s_on_lane; // descending
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
			const double s = vehicles[row].s_on_lane;
			auto it_ins = std::lower_bound(
				v_tgt.begin(), v_tgt.end(), s,
				[&](std::size_t j, double pos) {
					return vehicles[j].s_on_lane > pos;
				});

			v_tgt.insert(it_ins, row);
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

		void phase1_simd(
			TrafficSimulationSystem& system,
			std::vector<Vehicle>& vehicles,
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

			const idm::idm_params_t idm_def {}; // default calibrated parameters

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
					if (VehicleStateBitsV::has_info(vehicles[i].state, VehicleStateBits::FL_ERROR)) {
						continue;
					}

					// ─── 1. Gather follower state ────────────────────────────────────
					const float s_f = static_cast<float>(vehicles[i].s_on_lane); // [m]
					const float v_f = vehicles[i].currentSpeed; // [m/s]
					const float l_f = vehicles[i].length; // bumper‑to‑bumper length [m]

					// ─── 1b. Gather leader state (if any) ────────────────────────────
					float s_gap = 1e9f;   // sentinel = "free road"
					float v_leader = v_f; // same speed → Δv = 0

					if (k > 0) {
						const size_t j = idx[k - 1];
						const float s_l = static_cast<float>(vehicles[j].s_on_lane);
						v_leader = vehicles[j].currentSpeed;

						const float s_l_ctr = static_cast<float>(vehicles[j].s_on_lane);
						const float v_leader = vehicles[j].currentSpeed;
						const float len_leader = vehicles[j].length;

						s_gap = idm::actual_gap(s_l_ctr, s_f, len_leader, l_f);
					}

					// ─── 2. IDM acceleration ────────────────────────────────────────
					const float a = idm::idm_scalar(v_f, v_leader, s_gap, idm_def);

					// ─── 3. Kinematics update (Euler forward) ────────────────────────
					const float v_next = std::clamp(v_f + a * static_cast<float>(dt), 0.0f, rt.max_speed);
					vehicles[i].v_next = v_next;
					vehicles[i].s_next = s_f + v_f * dt + 0.5f * a * static_cast<float>(dt * dt);

					// ─── 4. Lane‑change decision (unchanged, but uses new kinematics) ─
					const float dist_to_node = rt.length - s_f;
					const AgentData& ag = agents[i];

					const uint16_t change_state = static_cast<int>(VehicleStateBits::ST_PREPARE) | static_cast<int>(VehicleStateBits::ST_CROSS) | static_cast<int>(VehicleStateBits::ST_ALIGN);
					if (!VehicleStateBitsV::has_any(vehicles[i].state, change_state, VehicleStateBitsDivision::STATE)) {
						if (i == VEHICLE_ID) {
							std::cout << "";
						}

						// Current and (first) desired lane indices on this edge
						const int curr_idx = rt.static_lane->index_in_edge; // 0 = right-most
						int goal_idx = -1;
						// continue with the current lane if we could
						if (((ag.goal_lane_mask >> curr_idx) & 1) == 0) {
							for (int b = 0; b < 32; ++b) { // first set bit in goal mask
								if ((ag.goal_lane_mask >> b) & 1) {
									goal_idx = b;
									break;
								}
							}
						} else {
							goal_idx = curr_idx;
						}

						if (goal_idx >= 0 && goal_idx != curr_idx && !VehicleStateBitsV::has_info(vehicles[i].state, VehicleStateBits::FL_COOLDOWN)) {
							const int lanes_delta = goal_idx - curr_idx; // +ve ⇒ need to go LEFT
							const float prep = D_PREP + std::abs(lanes_delta) * D_PREP_PER_LANE;

							if (dist_to_node < prep) {
								Lane* neigh = (lanes_delta > 0) ? rt.static_lane->left() : rt.static_lane->right();

								if (neigh) {
									vehicles[i].lane_target = neigh; // step one lane toward goal
									VehicleStateBitsV::set_info(vehicles[i].state, VehicleStateBits::ST_PREPARE, VehicleStateBitsDivision::STATE);
									vehicles[i].lane_change_time = 0.0f;
								}
							}
						}
					}

					// ─── 5. Cool‑down bookkeeping (unchanged) ────────────────────────
					if (VehicleStateBitsV::has_info(vehicles[i].state, VehicleStateBits::FL_COOLDOWN)) {
						float t = vehicles[i].lane_change_time;
						t += static_cast<float>(dt);
						if (t > T_MIN) {
							VehicleStateBitsV::remove_info(vehicles[i].state, VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);
							t = 0.f;
						}
						vehicles[i].lane_change_time = t;
					}
				}
			}
		}

		void phase2_commit(
			TrafficSimulationSystem& system,
			std::vector<Vehicle>& vehicles,
			std::vector<LaneRuntime>& lane_rt,
			const double dt) // unchanged param list
		{
			TJS_TRACY_NAMED("VehicleMovement::IDM::Phase2");

#if TJS_SIMULATION_DEBUG
			auto& debug = system.settings().debug_data;
#endif

			auto& agents = system.agents();

			// Swap current and next values for all vehicles
			for (auto& vehicle : vehicles) {
				std::swap(vehicle.s_on_lane, vehicle.s_next);
				std::swap(vehicle.currentSpeed, vehicle.v_next);
			}

			static const idm::idm_params_t p_idm {};

			// Extra structure so first cycle could be constant with indices
			struct PendingMove {
				std::size_t row;
				Lane* src;
				Lane* tgt;
			};

			std::vector<PendingMove> pending_moves;
			// Suppose that 10% will be moved in one tick
			pending_moves.reserve(agents.size() / 10);

			/* ---------------- lateral loop --------------------------------------- */
			for (const LaneRuntime& rt : lane_rt) {
				for (std::size_t row : rt.idx) {
					if (VehicleStateBitsV::has_info(vehicles[row].state, VehicleStateBits::ST_STOPPED)) {
						vehicles[row].lane_target = nullptr;
						continue;
					}

					Lane* tgt = vehicles[row].lane_target;

					if (row == VEHICLE_ID) {
						std::cout << "";
					}

					if (VehicleStateBitsV::has_info(vehicles[row].state, VehicleStateBits::ST_PREPARE) && tgt) {
						if (row == VEHICLE_ID) {
							std::cout << "";
						}

						vehicles[row].lane_change_time += static_cast<float>(dt);
						bool ready = vehicles[row].lane_change_time >= T_PREPARE;
						bool gap_ok_simple = false;
						bool politeness = false;

						if (ready) {
							/* ---- leader on target lane ----------------------------------- */
							const LaneRuntime& tgt_rt = lane_rt[tgt->index_in_buffer];
							const auto& idx = tgt_rt.idx;
							auto it = std::lower_bound(idx.begin(), idx.end(), vehicles[row].s_on_lane,
								[&](std::size_t j, double pos) { return vehicles[j].s_on_lane > pos; });
							float gap_lead = std::numeric_limits<float>::infinity();
							float v_lead = vehicles[row].currentSpeed;
							if (it != idx.begin()) {
								std::size_t j_lead = *(it - 1);
								gap_lead = idm::actual_gap(static_cast<float>(vehicles[j_lead].s_on_lane),
									static_cast<float>(vehicles[row].s_on_lane),
									vehicles[j_lead].length, vehicles[row].length);
								v_lead = vehicles[j_lead].currentSpeed;
							}

							/* ---- leader on current lane --------------------------------- */
							const auto& idx_curr = rt.idx;
							auto it_c = std::find(idx_curr.begin(), idx_curr.end(), row);
							float gap_curr = std::numeric_limits<float>::infinity();
							float v_lead_curr = vehicles[row].currentSpeed;
							if (it_c != idx_curr.begin()) {
								std::size_t j_curr = *(it_c - 1);
								gap_curr = idm::actual_gap(static_cast<float>(vehicles[j_curr].s_on_lane),
									static_cast<float>(vehicles[row].s_on_lane),
									vehicles[j_curr].length, vehicles[row].length);
								v_lead_curr = vehicles[j_curr].currentSpeed;
							}

							float a_old = idm::idm_scalar(vehicles[row].currentSpeed, v_lead_curr, gap_curr, p_idm);
							float a_new = idm::idm_scalar(vehicles[row].currentSpeed, v_lead, gap_lead, p_idm);
							const float benefit = a_new - a_old;

							bool mandatory = true;
							politeness = mandatory ? true : benefit > POLITENESS_THRESHOLD;

							float req_gap = std::max(TAU * vehicles[row].currentSpeed + DELTA, MIN_GAP);
							gap_ok_simple = gap_lead >= req_gap;
						}

						if (ready && politeness && gap_ok_simple) {
							pending_moves.push_back(PendingMove { row, rt.static_lane, tgt });
							vehicles[row].current_lane = tgt;
							vehicles[row].lane_target = nullptr;

							// hack for lane orientation; if it directed to right lane_change_dir should be -1:1, if right vice verse
							const auto& end_node = tgt->parent->end_node;
							const auto& start_node = tgt->parent->start_node;
							const double x_delta = std::fabs(end_node->coordinates.x - start_node->coordinates.x);
							const double y_delta = std::fabs(end_node->coordinates.y - start_node->coordinates.y);
							const bool decision_by_y = x_delta < y_delta;
							bool dir_right = decision_by_y ? start_node->coordinates.y < end_node->coordinates.y : start_node->coordinates.x > end_node->coordinates.x;
							const int edge_dir = dir_right ? -1 : 1;

							vehicles[row].lane_change_dir = static_cast<int8_t>((tgt->index_in_edge > rt.static_lane->index_in_edge) ? -1 : 1) * edge_dir;
							vehicles[row].lateral_offset = static_cast<float>(vehicles[row].lane_change_dir) * static_cast<float>(tgt->width);
							vehicles[row].lane_change_time = 0.0f;
							VehicleStateBitsV::overwrite_info(vehicles[row].state, VehicleStateBits::ST_CROSS, VehicleStateBitsDivision::STATE);

							if (row == VEHICLE_ID) {
								std::cout << "";
							}
							continue;
						}
					} else if (VehicleStateBitsV::has_info(vehicles[row].state, VehicleStateBits::ST_CROSS)) {
						vehicles[row].lane_change_time += static_cast<float>(dt);
						float prog = std::min(vehicles[row].lane_change_time / T_CROSS, 1.0f);
						float cos_term = std::cos(static_cast<float>(tjs::core::MathConstants::M_PI) * 0.5f * prog);
						vehicles[row].lateral_offset = static_cast<float>(vehicles[row].lane_change_dir) * static_cast<float>(vehicles[row].current_lane->width) * cos_term;
						if (prog >= 1.0f) {
							vehicles[row].lateral_offset = 0.0f;
							vehicles[row].lane_change_time = 0.0f;
							VehicleStateBitsV::overwrite_info(vehicles[row].state, VehicleStateBits::ST_ALIGN, VehicleStateBitsDivision::STATE);
						}
					} else if (VehicleStateBitsV::has_info(vehicles[row].state, VehicleStateBits::ST_ALIGN)) {
						if (row == VEHICLE_ID) {
							std::cout << "";
						}
						vehicles[row].lane_change_time += static_cast<float>(dt);
						if (vehicles[row].lane_change_time >= T_ALIGN) {
							VehicleStateBitsV::overwrite_info(vehicles[row].state, VehicleStateBits::ST_FOLLOW, VehicleStateBitsDivision::STATE);
							VehicleStateBitsV::set_info(vehicles[row].state, VehicleStateBits::FL_COOLDOWN, VehicleStateBitsDivision::FLAGS);
							vehicles[row].lane_change_time = 0.0f;
						}
					}
				}
			}

			/* ----------------  Do all moves after scanning--------------------------------------- */
			for (const auto& m : pending_moves) {
				idm::move_index(m.row, lane_rt, m.src, m.tgt, vehicles);
			}

			using VSB = VehicleStateBits;
			using DIV = VehicleStateBitsDivision;
			using bit_t = uint16_t;

			/* A small helper to test whether a car is still measurably off‑centre. */
			const auto off_centre = [&vehicles](std::size_t row) noexcept {
				return std::fabs(vehicles[row].lateral_offset) > 0.5f; // 50 cm tolerance
			};

			/* ---------------- edge hop loop -------------------------------------- */
			for (std::size_t i = 0; i < agents.size(); ++i) {
				AgentData& ag = agents[i];
				if (ag.path.empty()) {
					continue;
				}

				double remain = vehicles[i].s_on_lane;
				Lane* lane = vehicles[i].current_lane;

				TJS_BREAK_IF(
					debug.movement_phase == SimulationMovementPhase::IDM_Phase2_Agent
					&& i == debug.agent_id
					&& debug.lane_id == lane->get_id()
					&& debug.vehicle_indices == lane_rt[lane->index_in_buffer].idx);

				while (remain >= lane->length - 1e-6) {
					if (i == VEHICLE_ID) {
						std::cout << "";
					}
					remain -= lane->length;

					++ag.path_offset;
					if (ag.path_offset >= ag.path.size()) {
						stop_moving(i, ag, vehicles[i], lane, VehicleMovementError::ER_NO_PATH);
						break;
					}

					Edge* next_edge = ag.path[ag.path_offset];
					VehicleMovementError err;

					TJS_BREAK_IF(
						debug.movement_phase == SimulationMovementPhase::IDM_Phase2_ChooseLane
						&& i == debug.agent_id
						&& debug.lane_id == lane->get_id()
						&& debug.vehicle_indices == lane_rt[lane->index_in_buffer].idx);

					Lane* entry = choose_entry_lane(lane, next_edge, err);
					if (err != VehicleMovementError::ER_NO_ERROR || !entry) {
						stop_moving(i, ag, vehicles[i], lane, err);
						break;
					}

					if (!gap_ok(lane_rt[entry->index_in_buffer],
							vehicles,
							remain, vehicles[i].length,
							p_idm, dt, i)) {
						vehicles[i].s_on_lane = lane->length - 0.01;
						vehicles[i].s_next = vehicles[i].s_on_lane;
						break;
					}

					/* -----------------------------------------------
					* Avoid node transitions while a lateral
					* manoeuvre (prepare / cross / align) is active.
					* --------------------------------------------- */
					// Changing lane in this cycle has more priority over changing lane in lateral movement because
					bit_t change_mask = (bit_t)VSB::ST_PREPARE | (bit_t)VSB::ST_CROSS;
					bool busy = VehicleStateBitsV::has_any(vehicles[i].state, change_mask, DIV::STATE) || (VehicleStateBitsV::has_info(vehicles[i].state, VSB::ST_ALIGN) && off_centre(i));
					if (busy) {
						if (remain >= lane->length - 1e-3) {
							vehicles[i].s_on_lane = vehicles[i].s_next = lane->length - 1e-3;
						}
						break;
					}

					/* ----- commit hop ------------------------------------------- */
					vehicles[i].s_on_lane = remain;
					idm::move_index(i, lane_rt, lane, entry, vehicles);
					lane = entry;
					vehicles[i].current_lane = entry;

					if (ag.path_offset < ag.path.size() - 1) {
						ag.goal_lane_mask = build_goal_mask(*entry->parent, *ag.path[ag.path_offset + 1]);
					} else {
						ag.goal_lane_mask = 0xFFFF;
					}
					/* ----- SUMO‑style speed clamp ------------------------------ */
					const auto& tgt_idx = lane_rt[entry->index_in_buffer].idx;
					if (!tgt_idx.empty() && tgt_idx.front() != i) {
						std::size_t j_lead = tgt_idx.front();
						float gap_leader = idm::actual_gap(static_cast<float>(vehicles[j_lead].s_on_lane),
							static_cast<float>(vehicles[i].s_on_lane),
							vehicles[j_lead].length, vehicles[i].length);
						float v_safe = idm::safe_entry_speed(vehicles[j_lead].currentSpeed, gap_leader, dt);
						vehicles[i].currentSpeed = std::clamp(v_safe, 0.0f, vehicles[i].currentSpeed);
					}
				}
			}
		}
	} // namespace idm
} // namespace tjs::core::simulation
