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
		TJS_TRACY_NAMED("VehicleMovement_Update");
		auto& agents = _system.agents();

		for (size_t i = 0; i < agents.size(); ++i) {
			movement_details::update_agent(agents[i], _system);
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

	namespace movement_details {

		void check_move_beginning(AgentData& agent, TrafficSimulationSystem& system) {
			Vehicle& vehicle = *agent.vehicle;
			if (vehicle.state == VehicleState::PendingMove) {
				auto parent_edge = vehicle.current_lane->parent;
				if (!agent.path.empty()) {
					auto target_edge = agent.path.front();
					if (
						parent_edge != target_edge
						&& parent_edge->start_node == target_edge->start_node) {
						vehicle.current_lane = &target_edge->lanes[0];
					} else {
						// TODO[simulation]: error handling when cannot find out edge
						vehicle.current_lane = &target_edge->lanes[0];
					}
					vehicle.s_on_lane = 0.f;
					agent.path.erase(agent.path.begin());
				}
			}
		}

		void adjust_lane(AgentData& agent, TrafficSimulationSystem& system) {
			Vehicle& vehicle = *agent.vehicle;
			if (vehicle.current_lane != nullptr) {
				return;
			}
			auto& world = system.worldData();
			auto& segment = world.segments().front();
			auto& road_network = *segment->road_network;

			// TODO: REMOVE HACK
			vehicle.current_lane = find_lane(vehicle.coordinates, road_network);
		}

		void adjust_speed(Vehicle& vehicle) {
			const Lane* lane = vehicle.current_lane;
			vehicle.currentSpeed = std::min(
				static_cast<float>(lane->parent->way->maxSpeed),
				vehicle.maxSpeed);
		}

		void move_vehicle(Vehicle& vehicle, TrafficSimulationSystem& system) {
			double delta_time = system.timeModule().state().fixed_dt();
			const double speed_mps = vehicle.currentSpeed * 1000.0 / 3600.0;
			const double max_move = speed_mps * delta_time;

			const Lane* lane = vehicle.current_lane;
			const auto& start = lane->centerLine.front();
			const auto& end = lane->centerLine.back();

			double remaining = lane->length - vehicle.s_on_lane;
			double move = std::min(max_move, remaining);
			vehicle.s_on_lane += move;

			vehicle.coordinates = move_towards(start, end, vehicle.s_on_lane, lane->length);

			core::Coordinates dir {};
			dir.x = end.x - start.x;
			dir.y = end.y - start.y;
			vehicle.rotationAngle = atan2(dir.y, dir.x);
		}

		void check_next_target(AgentData& agent, TrafficSimulationSystem& system) {
			auto& vehicle = *agent.vehicle;
			if (vehicle.s_on_lane < vehicle.current_lane->length) {
				return;
			}

			// TODO[simulation]: make in define and debug data
			//if (agent.current_goal->end_node->uid == 16) {
			//_system.timeModule().pause();
			//}

			if (agent.path.empty()) {
				vehicle.state = VehicleState::Stopped;
				vehicle.error = MovementError::NoPath;
				return;
			}

			const auto& outgoing = vehicle.current_lane->outgoing_connections;

			Lane* next_lane = nullptr;
			auto& next_edge = *agent.path.front();
			for (auto& link : outgoing) {
				if (link->to->parent == &next_edge) {
					next_lane = link->to;
					break;
				}
			}

			// Try to teleport to another lane
			// in future should be changed by TacticalModule + change lane mechanism)
			if (!next_lane) {
				Lane* current_teleport = nullptr;
				for (auto& lane : next_edge.lanes) {
					for (auto& link : lane.incoming_connections) {
						if (link->from->parent == vehicle.current_lane->parent) {
							current_teleport = link->from;
							next_lane = &lane;
							break;
						}
					}
				}
				if (current_teleport) {
					vehicle.current_lane = current_teleport;
					vehicle.s_on_lane = current_teleport->length;
					vehicle.coordinates = move_towards(
						vehicle.current_lane->centerLine.front(),
						vehicle.current_lane->centerLine.back(),
						vehicle.s_on_lane,
						vehicle.current_lane->length);
				}
			}

			if (next_lane) {
				vehicle.current_lane = next_lane;
				vehicle.s_on_lane = 0;
				vehicle.coordinates = move_towards(
					vehicle.current_lane->centerLine.front(),
					vehicle.current_lane->centerLine.back(),
					vehicle.s_on_lane,
					vehicle.current_lane->length);

				agent.path.erase(agent.path.begin());
			} else {
				vehicle.state = VehicleState::Stopped;
				vehicle.error = outgoing.empty() ? MovementError::NoOutgoingConnections : MovementError::NoNextLane;
			}
		}

		void process_vehicle_state(AgentData& agent, TrafficSimulationSystem& system) {
			auto& vehicle = *agent.vehicle;
			switch (vehicle.state) {
				case VehicleState::PendingMove: {
					check_move_beginning(agent, system);
					adjust_lane(agent, system);
					vehicle.state = VehicleState::Moving;
				} break;
				case VehicleState::Moving: {
					// TODO[simulation]: cycle for movement with different lanes
					adjust_speed(vehicle);
					move_vehicle(vehicle, system);
					check_next_target(agent, system);
				} break;
				case VehicleState::Stopped:
				case VehicleState::Undefined:
				case VehicleState::Count:
					break;
			}
		}

		void update_agent(AgentData& agent, TrafficSimulationSystem& system) {
			if (agent.currentGoal == nullptr) {
				return;
			}
			process_vehicle_state(agent, system);
		}
	} // namespace movement_details

} // namespace tjs::core::simulation
