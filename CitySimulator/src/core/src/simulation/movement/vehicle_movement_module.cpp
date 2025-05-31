#include <core/stdafx.h>

#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/simulation/simulation_system.h>
#include <core/math_constants.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::simulation {
	VehicleMovementModule::VehicleMovementModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void VehicleMovementModule::initialize() {
	}

	void VehicleMovementModule::release() {
	}

	void VehicleMovementModule::update() {
		auto& agents = _system.agents();

		for (size_t i = 0; i < agents.size(); ++i) {
			update_movement(agents[i]);
		}
	}

	static core::Coordinates move_towards(
		const core::Coordinates& start,
		const core::Coordinates& end,
		double distance,
		double total_distance // precalculated value
	) {
		if (distance <= 0) {
			return start;
		}

		//const double total_dist = core::algo::haversine_distance(start, end);
		if (total_distance <= 1e-3) {
			return end;
		}

		const double fraction = distance / total_distance;

		const double lat = start.latitude + fraction * (end.latitude - start.latitude);
		const double lon = start.longitude + fraction * (end.longitude - start.longitude);

		return { lat, lon };
	}

	void VehicleMovementModule::update_movement(AgentData& agent) {
		auto& worldData = _system.worldData();

		agent.vehicle->currentSpeed = 60.0f;
		double delta_time = _system.timeModule().state().timeDelta;
		// Convert km/h to m/s and calculate distance covered this frame
		const double speed_mps = agent.vehicle->currentSpeed * 1000.0 / 3600.0;
		const double max_move = speed_mps * delta_time;

		// Get current and target positions
		const core::Coordinates current = agent.vehicle->coordinates;
		const core::Coordinates target = agent.currentStepGoal;

		// Calculate actual movement
		const double distance_to_target = core::algo::haversine_distance(current, target);

		if (distance_to_target <= max_move) {
			// Reached destination
			agent.vehicle->coordinates = target;
		} else {
			// Calculate new position
			agent.vehicle->coordinates = move_towards(current, target, max_move, distance_to_target);
		}

		// Ensure speed doesn't exceed maximum
		agent.vehicle->currentSpeed = std::min(
			agent.vehicle->currentSpeed,
			agent.vehicle->maxSpeed);

		core::Coordinates dir {
			target.latitude - current.latitude,
			target.longitude - current.longitude
		};
		agent.vehicle->rotationAngle = atan2(dir.longitude, dir.latitude);
	}

} // namespace tjs::core::simulation
