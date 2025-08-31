#include <core/stdafx.h>

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/data_layer/vehicle.h>

namespace tjs::core::simulation {
	/*void VehicleBuffers::clear() {
		s_curr.clear();
		s_next.clear();
		v_curr.clear();
		v_next.clear();
		desired_v.clear();
		length.clear();
		lateral_off.clear();
		lane.clear();
		lane_target.clear();
		lane_after.clear();
		action_time.clear();
		lane_change_dir.clear();
		flags.clear();
		uids.clear();
	}

	void VehicleBuffers::reserve(size_t count) {
		s_curr.reserve(count);
		s_next.reserve(count);
		v_curr.reserve(count);
		v_next.reserve(count);
		desired_v.reserve(count);
		length.reserve(count);
		lateral_off.reserve(count);
		lane.reserve(count);
		lane_target.reserve(count);
		lane_after.reserve(count);
		action_time.reserve(count);
		lane_change_dir.reserve(count);
		flags.reserve(count);
		uids.reserve(count);
	}

	void VehicleBuffers::add_vehicle(Vehicle& vehicle) {
		s_curr.push_back(vehicle.s_on_lane);
		s_next.push_back(vehicle.s_on_lane);
		v_curr.push_back(vehicle.currentSpeed);
		v_next.push_back(vehicle.currentSpeed);
		desired_v.push_back(vehicle.maxSpeed);
		length.push_back(vehicle.length);
		lateral_off.push_back(vehicle.lateral_offset);
		lane.push_back(vehicle.current_lane);
		lane_target.push_back(nullptr);
		lane_after.push_back(nullptr);
		action_time.push_back(0.0f);
		lane_change_dir.push_back(0);
		flags.push_back(vehicle.state);
		uids.push_back(vehicle.uid);
	}*/
} // namespace tjs::core::simulation
