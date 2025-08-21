#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

namespace tjs::core {

	struct WayInfo;
	struct Lane;
	struct AgentData;

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
		 // ---- 8‑byte group (kept together to avoid padding) ----
		uint64_t uid;

		double   s_on_lane;
		double   s_next;
		double   lateral_offset;

		Coordinates coordinates;
		WayInfo*  currentWay;
		Lane*     current_lane;
		Lane*     lane_target;
		AgentData* agent;

		// ---- 4‑byte group ----
		float currentSpeed;
		float v_next;
		float rotationAngle;
		float lane_change_time;
		float length;
		float width;
		float maxSpeed;
		int   currentSegmentIndex;

		VehicleType type;
		simulation::VehicleMovementError error;

		// ---- 2‑byte group ----
		uint16_t state;
		uint16_t previous_state;

		// ---- 1‑byte group ----
		int8_t lane_change_dir;
		bool   has_position_changes;
	};
	static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

	using Vehicles = std::vector<Vehicle>;

} // namespace tjs::core
