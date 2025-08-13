#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

namespace tjs::core {

	struct WayInfo;
	struct Lane;

	enum class VehicleType : char {
		SimpleCar,
		SmallTruck,
		BigTruck,
		Ambulance,
		PoliceCar,
		FireTrack,

		Count
	};

	ENUM(VehicleState, uint8_t,
		Undefined, PendingMove, Moving, Stopped);

	struct Vehicle {
		uint64_t uid;
		float currentSpeed;
		float maxSpeed;
		Coordinates coordinates;
		VehicleType type;
		float length;
		float width;
		WayInfo* currentWay;
		int currentSegmentIndex;
		float rotationAngle; // orientation in radians
		Lane* current_lane;
		double s_on_lane;
		double lateral_offset;
		simulation::VehicleMovementError error;
		uint16_t state;
		uint16_t previous_state;

		// Additional fields for lane agnostic movement calculations
		double s_next;           // next position on lane
		float v_next;            // next speed
		Lane* lane_target;       // target lane for lane change
		float lane_change_time;  // timer for lane change FSM
		int8_t lane_change_dir;  // +1 leftwards, -1 rightwards
	};
	static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

	using Vehicles = std::vector<Vehicle>;

} // namespace tjs::core
