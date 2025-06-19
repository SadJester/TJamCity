#include <core/stdafx.h>

#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/simulation/simulation_system.h>
#include <core/math_constants.h>
#include <core/map_math/earth_math.h>
#include <core/data_layer/road_network.h>

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
		if (agent.currentGoal == nullptr || agent.vehicle->currentLane == nullptr) {
			return;
		}

		agent.vehicle->currentSpeed = 60.0f;
		double delta_time = _system.timeModule().state().timeDelta;
		const double speed_mps = agent.vehicle->currentSpeed * 1000.0 / 3600.0;
		const double max_move = speed_mps * delta_time;

		Lane* lane = agent.vehicle->currentLane;
		const auto& start = lane->centerLine.front();
		const auto& end = lane->centerLine.back();
		double lane_length = core::algo::haversine_distance(start, end);

		double remaining = lane_length - agent.vehicle->s_on_lane;
		double move = std::min(max_move, remaining);
		agent.vehicle->s_on_lane += move;

		agent.vehicle->coordinates = move_towards(start, end, agent.vehicle->s_on_lane, lane_length);

		agent.vehicle->currentSpeed = std::min(agent.vehicle->currentSpeed, agent.vehicle->maxSpeed);

		core::Coordinates dir {
			end.latitude - start.latitude,
			end.longitude - start.longitude
		};
		agent.vehicle->rotationAngle = atan2(dir.longitude, dir.latitude);

		if (agent.vehicle->s_on_lane >= lane_length) {
			agent.vehicle->currentLane = agent.target_lane ? agent.target_lane : agent.vehicle->currentLane;
			agent.vehicle->s_on_lane = 0.0;
		}
	}

} // namespace tjs::core::simulation
