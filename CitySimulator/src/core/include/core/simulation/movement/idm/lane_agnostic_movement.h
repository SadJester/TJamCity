#pragma once

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/simulation/movement/movement_runtime_structures.h>

#include <core/simulation/movement/idm/idm_params.h>

namespace tjs::core {
	struct Lane;
	struct Edge;
	struct AgentData;
} // namespace tjs::core

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	struct VehicleBuffers;

	namespace idm {
		void phase1_simd(
			TrafficSimulationSystem& system,
			VehicleBuffers& buf,
			const std::vector<LaneRuntime>& lane_rt,
			double dt);

		void phase2_commit(
			TrafficSimulationSystem& system,
			VehicleBuffers& buf,
			std::vector<LaneRuntime>& lane_rt,
			double dt);

		// Scalar Intelligent‑Driver‑Model acceleration.  Used by unit tests and as a
		// readable reference for the SIMD drop‑in.
		//
		//   v_follower – current speed of vehicle [m/s]
		//   v_leader   – speed of leader [m/s]
		//   s_gap      – actual bumper‑to‑bumper distance [m]
		//   p          – calibrated IDM parameters
		float idm_scalar(const float v_follower, const float v_leader, const float s_gap, const idm_params_t& p) noexcept;

		//------------------------------------------------------------------
		//  move_index
		//     • row          = VehicleBuffers row to move
		//     • lane_rt      = global vector<LaneRuntime> (indexed by Lane::id())
		//     • src / tgt    = source & destination Lane* (may be identical)
		//     • s_curr       = reference to the current-position column
		//
		//  Effect: removes `row` from src-lane's idx vector and inserts it in
		//          order into tgt-lane's idx vector so both stay sorted.
		//------------------------------------------------------------------
		void move_index(std::size_t row,
			std::vector<LaneRuntime>& lane_rt,
			const Lane* src,
			const Lane* tgt,
			const std::vector<double>& s_curr);

		//------------------------------------------------------------------
		//  Pick the concrete landing lane on `next_edge` for a car that is
		//  currently in `src_lane` and about to cross the node.
		//
		//  • Must return a valid pointer (throws if the route is impossible).
		//  • Preference order:   1) non-yield link
		//                        2) shortest lateral hop (|id diff|)
		//                        3) first found
		//------------------------------------------------------------------
		Lane* choose_entry_lane(const Lane* src_lane, const Edge* next_edge, VehicleMovementError& err);
	} // namespace idm
} // namespace tjs::core::simulation
