#pragma once

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

	ENUM(MovementError, char,
		None,
		NoOutgoingConnections,
		NoPath,
		NoNextLane);

	ENUM(VehicleState, uint8_t,
		Undefined, PendingMove, Moving, Stopped)

	struct Vehicle {
		uint64_t uid;
		float currentSpeed;
		float maxSpeed;
		Coordinates coordinates;
		VehicleType type;
		WayInfo* currentWay;
		int currentSegmentIndex;
		float rotationAngle; // orientation in radians
		Lane* current_lane;
		double s_on_lane;
		double lateral_offset;
		VehicleState state;
		MovementError error;
	};
	static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

} // namespace tjs::core
