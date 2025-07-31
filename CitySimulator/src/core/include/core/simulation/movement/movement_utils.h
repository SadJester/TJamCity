#pragma once

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/simulation/movement/movement_runtime_structures.h>

namespace tjs::core {
	struct Lane;
	struct Edge;
	struct AgentData;
} // namespace tjs::core

namespace tjs::core::simulation {
	// ------------------------------------------------------------------
	// Build a 32-bit mask for *current* edge that flags which lanes can
	// exit into `next_edge` according to LaneLink table.
	// ------------------------------------------------------------------
	uint32_t build_goal_mask(const Edge& curr_edge, const Edge& next_edge);
	void stop_moving(size_t i, AgentData& ag, VehicleBuffers& buf, Lane* lane, VehicleMovementError error);
} // namespace tjs::core::simulation
