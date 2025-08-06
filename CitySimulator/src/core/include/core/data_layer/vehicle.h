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
	};
	static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

	using Vehicles = std::vector<Vehicle>;

} // namespace tjs::core
