#include <core/stdafx.h>

#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/simulation/simulation_system.h>
#include <core/math_constants.h>
#include <core/data_layer/world_data.h>
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

		//const double total_dist = core::algo::euclidean_distance(start, end);
		if (total_distance <= 1e-3) {
			return end;
		}

		const double fraction = distance / total_distance;

		const double x = start.x + fraction * (end.x - start.x);
		const double y = start.y + fraction * (end.y - start.y);

		Coordinates result {};
		result.x = x;
		result.y = y;
		return result;
	}

	const Lane* find_lane(const Coordinates& coordinates, const RoadNetwork& network) {
		double min_distance = std::numeric_limits<double>::max();
		const Lane* candidate = nullptr;
		for (auto& edge : network.edges) {
			for (auto& lane : edge.lanes) {
				double distance = core::algo::euclidean_distance(lane.parent->start_node->coordinates, coordinates);
				if (distance > 15.0 && distance < min_distance) {
					min_distance = distance;
					candidate = &lane;
				}
			}
		}
		return candidate;
	}

	void VehicleMovementModule::update_movement(AgentData& agent) {
		if (agent.currentGoal == nullptr || agent.vehicle->current_lane == nullptr) {
			return;
		}

		auto vehicle = agent.vehicle;
		if (vehicle->state == VehicleState::Stopped) {
			return;
		}

		// adjust lane
		// TODO: here can be bug due to changing lanes with teleport
		if (agent.target_lane != vehicle->current_lane) {
			if (vehicle->current_lane != nullptr) {
				// hack for adjusting first lane (but probabely it can be not only on first segment)
				if (agent.target_lane->parent->start_node == vehicle->current_lane->parent->start_node) {
					vehicle->current_lane = agent.target_lane;
				}
			}
		}

		agent.vehicle->state = VehicleState::Moving;

		if (agent.vehicle->current_lane == nullptr) {
			auto& world = _system.worldData();
			auto& segment = world.segments().front();
			auto& road_network = *segment->road_network;

			// TODO: REMOVE HACK
			agent.vehicle->current_lane = find_lane(agent.vehicle->coordinates, road_network);
		}

		const Lane* lane = agent.vehicle->current_lane;
		agent.vehicle->currentSpeed = 60.0f; //std::min(static_cast<float>(lane->parent->way->maxSpeed), 60.0f);

		double delta_time = _system.timeModule().state().timeDelta;
		const double speed_mps = agent.vehicle->currentSpeed * 1000.0 / 3600.0;
		const double max_move = speed_mps * delta_time;

		const auto& start = lane->centerLine.front();
		const auto& end = lane->centerLine.back();

		double remaining = lane->length - agent.vehicle->s_on_lane;
		double move = std::min(max_move, remaining);
		agent.vehicle->s_on_lane += move;

		agent.vehicle->coordinates = move_towards(start, end, agent.vehicle->s_on_lane, lane->length);

		agent.vehicle->currentSpeed = std::min(agent.vehicle->currentSpeed, agent.vehicle->maxSpeed);

		core::Coordinates dir {};
		dir.x = end.x - start.x;
		dir.y = end.y - start.y;
		agent.vehicle->rotationAngle = atan2(dir.y, dir.x);

		if (agent.vehicle->s_on_lane >= lane->length) {
			// Lane switching logic with multiple fallback mechanisms:
			Lane* next_lane = nullptr;
			if (!vehicle->current_lane->outgoing_connections.empty()) {
				// Find the first valid outgoing connection
				for (Lane* candidate : vehicle->current_lane->outgoing_connections) {
					if (candidate != nullptr) {
						for (const Lane& goal_lane : agent.current_goal->lanes) {
							if (candidate == &goal_lane) {
								next_lane = candidate;
								break;
							}
						}
					}
				}
			}

			if (next_lane) {
				auto prev_lane = vehicle->current_lane;
				auto dist = core::algo::euclidean_distance(vehicle->coordinates, next_lane->parent->start_node->coordinates);
				if (vehicle->current_lane == next_lane) {
					vehicle->s_on_lane = 0.0f;
				} else {
					vehicle->current_lane = next_lane;
					vehicle->s_on_lane = 0.0;
				}
				auto s = next_lane->centerLine[0];
				auto e = next_lane->centerLine[1];
				auto coords = move_towards(s, e, agent.vehicle->s_on_lane, lane->length);
				auto diff = core::algo::euclidean_distance(vehicle->coordinates, coords);
				if (diff > move) {
					s = e;
				}
			} else {
				if (vehicle->current_lane == agent.target_lane) {
					vehicle->s_on_lane = 1.0f;
				} else {
					// No connector found: block, stop, or fallback
					vehicle->state = VehicleState::Stopped;
				}
			}
		}
	}

} // namespace tjs::core::simulation
