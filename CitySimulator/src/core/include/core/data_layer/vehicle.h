#pragma once

#include <core/simulation/transport_management/vehicle_state.h>
#include <core/simulation/simulation_types.h>

#include <core/simulation/movement/idm/idm_params.h>

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

	struct ProfileInfo {
		MovementAlgoType algo;
		TacticalBehaviour behaviour;
		uint8_t uniquness;
	};

	struct Vehicle {
		ProfileInfo profile;
		// ---- 8‑byte group (kept together to avoid padding) ----
		uint64_t uid;

		double s_on_lane;
		double s_next;
		double lateral_offset;
		double action_time;

		Coordinates coordinates;
		WayInfo* currentWay;
		Lane* current_lane;
		Lane* lane_target;
		AgentData* agent;
		Vehicle* cooperation_vehicle;

		// ---- 4‑byte group ----
		float currentSpeed;
		float v_next;
		float rotationAngle;
		float length;
		float width;
		float maxSpeed;
		uint32_t goal_lane_mask; // bitmask for current edge exit
		int currentSegmentIndex;
		uint32_t idx_in_lane;
		uint32_t idx_in_target_lane;

		VehicleType type;
		simulation::VehicleMovementError error;

		// ---- 2‑byte group ----
		uint16_t state;
		uint16_t previous_state;

		// ---- 1‑byte group ----
		int8_t lane_change_dir;
		bool has_position_changes;

		// Return if vehicle is shadow in lane (behin to change lane)
		bool is_merging(const Lane& lane) const;
	};
	static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

	using Vehicles = std::vector<Vehicle>;

} // namespace tjs::core
