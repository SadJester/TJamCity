#pragma once

namespace tjs::core {
	struct Lane;
	struct Edge;
} // namespace tjs::core

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	struct LaneRuntime {
		Lane* static_lane = nullptr;
		double length;
		float max_speed;
		std::vector<std::size_t> idx;
	};

	struct EdgePrecomp {
		std::vector<uint32_t> lane_exit_mask;
	};

	struct VehicleBuffers;

	uint32_t build_goal_mask(const Edge& curr_edge, const Edge& next_edge);

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

} // namespace tjs::core::simulation
