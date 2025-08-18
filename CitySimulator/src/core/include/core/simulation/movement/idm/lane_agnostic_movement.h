#pragma once

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/simulation/movement/movement_runtime_structures.h>

#include <core/simulation/movement/idm/idm_params.h>

namespace tjs::core {
	struct Lane;
	struct Edge;
	struct AgentData;
	struct Vehicle;
} // namespace tjs::core

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	namespace idm {
		void phase1_simd(
			TrafficSimulationSystem& system,
			std::vector<Vehicle>& vehicles,
			const std::vector<LaneRuntime>& lane_rt,
			double dt);

		void phase2_commit(
			TrafficSimulationSystem& system,
			std::vector<Vehicle>& vehicles,
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
		//     • row          = Vehicle index to move
		//     • lane_rt      = global vector<LaneRuntime> (indexed by Lane::id())
		//     • src / tgt    = source & destination Lane* (may be identical)
		//     • vehicles     = reference to vehicles vector
		//
		//  Effect: removes `row` from src-lane's idx vector and inserts it in
		//          order into tgt-lane's idx vector so both stay sorted.
		//------------------------------------------------------------------
		void move_index(std::size_t row,
			std::vector<LaneRuntime>& lane_rt,
			const Lane* src,
			const Lane* tgt,
			std::vector<Vehicle>& vehicles);

		//------------------------------------------------------------------
		//  choose_entry_lane
		//     • src_lane     = current lane
		//     • next_edge    = target edge
		//     • err          = error output
		//
		//  Returns: best lane to enter on next_edge, or nullptr if impossible
		//------------------------------------------------------------------
		Lane* choose_entry_lane(const Lane* src_lane, const Edge* next_edge, VehicleMovementError& err);

	} // namespace idm
} // namespace tjs::core::simulation
