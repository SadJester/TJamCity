#include <core/stdafx.h>

#include <core/simulation/movement/movement_utils.h>

#include <core/simulation/agent/agent_data.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/vehicle.h>

namespace tjs::core::simulation {

	uint32_t build_goal_mask(const Edge& curr_edge, const Edge& next_edge) {
		uint32_t mask = 0;

		for (size_t i = 0; i < curr_edge.lanes.size(); ++i) {
			const Lane& ln = curr_edge.lanes[i];
			size_t from_idx = i;

			for (LaneLinkHandler h : ln.outgoing_connections) {
				const LaneLink& link = *h;
				if (link.to && link.to->parent == &next_edge) {
					mask |= (1u << from_idx); // mark this *source* lane
				}
			}
		}
		return mask; // 0 means “none of the lanes reach next_edge” → error
	}

	void stop_moving(size_t i, AgentData& ag, Vehicle& vehicle, Lane* lane, VehicleMovementError error) {
		VehicleStateBitsV::set_info(vehicle.state, VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
		VehicleStateBitsV::set_info(vehicle.state, VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);

		ag.vehicle->error = error;
		vehicle.s_next = lane->length - 0.01;
		vehicle.s_on_lane = vehicle.s_next;
		vehicle.lane_target = nullptr;

		// Stop vehicle at all, for other cases we need more sophisticated calculations
		// But for now treat that it will be movement further with the same speed as before
		if (lane->outgoing_connections.empty()) {
			vehicle.v_next = 0.0f;
			vehicle.currentSpeed = 0.0f;
		}
	}

} // namespace tjs::core::simulation
