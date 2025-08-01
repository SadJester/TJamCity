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
					break;                    // no need to scan others
				}
			}
		}
		return mask; // 0 means “none of the lanes reach next_edge” → error
	}

	void stop_moving(size_t i, AgentData& ag, VehicleBuffers& buf, Lane* lane, VehicleMovementError error) {
		VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
		VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);

		ag.vehicle->error = error;
		buf.s_curr[i] = lane->length - 0.01;
		buf.s_next[i] = buf.s_curr[i];
		buf.v_next[i] = buf.v_curr[i];

		// Stop vehicle at all, for other cases we need more sophisticated calculations
		// But for now treat that it will be movement further with the same speed as before
		if (lane->outgoing_connections.empty()) {
			buf.v_curr[i] = 0.0f;
			buf.v_next[i] = 0.0f;
		}
	}

} // namespace tjs::core::simulation
