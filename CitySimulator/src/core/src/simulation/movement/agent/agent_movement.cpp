#include <core/stdafx.h>

#include <core/simulation/movement/agent/agent_movement.h>

#include <core/simulation/simulation_system.h>

#include <core/math_constants.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/earth_math.h>
#include <core/data_layer/road_network.h>
#include <core/data_layer/lane_vehicle_utils.h>

namespace tjs::core::simulation {

	AgentMovementAlgo::AgentMovementAlgo(TrafficSimulationSystem& system)
		: IMovementAlgorithm(MovementAlgoType::Agent, system) {
	}

	void AgentMovementAlgo::update() {
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

	Lane* find_lane(const Coordinates& coordinates, RoadNetwork& network) {
		double min_distance = std::numeric_limits<double>::max();
		Lane* candidate = nullptr;
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
			/*Vehicle& vehicle = *agent.vehicle;
			if (vehicle.state == VehicleState::PendingMove) {
				auto parent_edge = vehicle.current_lane->parent;
				if (!agent.path.empty()) {
					auto target_edge = agent.path.front();
					if (
						parent_edge != target_edge
						&& parent_edge->start_node == target_edge->start_node) {
						Lane* old_lane = vehicle.current_lane;
						vehicle.current_lane = &target_edge->lanes[0];
						if (old_lane) {
							remove_vehicle(*old_lane, &vehicle);
						}
						insert_vehicle_sorted(*vehicle.current_lane, &vehicle);
					} else {
						// TODO[simulation]: error handling when cannot find out edge
						Lane* old_lane = vehicle.current_lane;
						vehicle.current_lane = &target_edge->lanes[0];
						if (old_lane) {
							remove_vehicle(*old_lane, &vehicle);
						}
						insert_vehicle_sorted(*vehicle.current_lane, &vehicle);
					}
					vehicle.s_on_lane = 0.f;
					agent.path.erase(agent.path.begin());
				}
			}*/
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
			if (vehicle.current_lane) {
				insert_vehicle_sorted(*vehicle.current_lane, &vehicle);
			}
		}

		void adjust_speed(Vehicle& vehicle) {
			const Lane* lane = vehicle.current_lane;
			vehicle.currentSpeed = std::min(
				static_cast<float>(lane->parent->way->maxSpeed),
				vehicle.maxSpeed);
		}

		void move_vehicle(Vehicle& vehicle, Lane& lane, double move) {
			const auto& start = lane.centerLine.front();
			const auto& end = lane.centerLine.back();

			vehicle.s_on_lane += move;
			vehicle.coordinates = move_towards(start, end, vehicle.s_on_lane, lane.length);

			core::Coordinates dir {};
			dir.x = end.x - start.x;
			dir.y = end.y - start.y;
			vehicle.rotationAngle = atan2(dir.y, dir.x);
		}

		void advance_vehicle(AgentData& agent, TrafficSimulationSystem& system) {
			Vehicle& vehicle = *agent.vehicle;
			double delta_time = system.timeModule().state().fixed_dt();
			double speed_mps = vehicle.currentSpeed * 1000.0 / 3600.0;
			double remaining_move = speed_mps * delta_time;

			/*while (remaining_move > 0 && vehicle.current_lane) {
				Lane* lane = vehicle.current_lane;
				double to_end = lane->length - vehicle.s_on_lane;
				double move = std::min(remaining_move, to_end);

				move_vehicle(vehicle, *lane, move);
				update_vehicle_position(*lane, &vehicle);

				remaining_move -= move;
				if (remaining_move <= 0) {
					break;
				}

				if (agent.path.empty()) {
					VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
					VehicleStateBitsV::set_info(buf.flags[i], VehicleStateBits::FL_ERROR, VehicleStateBitsDivision::FLAGS);
					vehicle.state = VehicleState::Stopped;
					vehicle.error = MovementError::NoPath;
					break;
				}

				const auto& outgoing = lane->outgoing_connections;
				Lane* next_lane = nullptr;
				auto& next_edge = *agent.path.front();
				for (auto& link : outgoing) {
					if (link->to->parent == &next_edge) {
						next_lane = link->to;
						break;
					}
				}

				if (!next_lane) {
					Lane* current_teleport = nullptr;
					for (auto& l : next_edge.lanes) {
						for (auto& link : l.incoming_connections) {
							if (link->from->parent == vehicle.current_lane->parent) {
								current_teleport = link->from;
								next_lane = &l;
								break;
							}
						}
					}
					if (current_teleport) {
						remove_vehicle(*lane, &vehicle);
						vehicle.current_lane = current_teleport;
						vehicle.s_on_lane = current_teleport->length;
						move_vehicle(vehicle, *vehicle.current_lane, 0.0);
						insert_vehicle_sorted(*vehicle.current_lane, &vehicle);
						lane = vehicle.current_lane;
					}
				}

				if (next_lane) {
					remove_vehicle(*lane, &vehicle);
					vehicle.current_lane = next_lane;
					vehicle.s_on_lane = 0;
					move_vehicle(vehicle, *vehicle.current_lane, 0.0);
					agent.path.erase(agent.path.begin());
					insert_vehicle_sorted(*vehicle.current_lane, &vehicle);
				} else {
					vehicle.state = VehicleState::Stopped;
					vehicle.error = outgoing.empty() ? MovementError::NoOutgoingConnections : MovementError::NoNextLane;
					break;
				}
			}*/
		}

		void process_vehicle_state(AgentData& agent, TrafficSimulationSystem& system) {
			auto& vehicle = *agent.vehicle;
			/*switch (vehicle.state) {
				case VehicleState::PendingMove: {
					check_move_beginning(agent, system);
					adjust_lane(agent, system);
					VehicleStateBitsV::set_info(vehicle.state_, VehicleStateBits::ST_FOLLOW, )
					vehicle.state_ = VehicleState::Moving;
				} break;
				case VehicleState::Moving: {
					// TODO[simulation]: cycle for movement with different lanes
					adjust_speed(vehicle);
					advance_vehicle(agent, system);
				} break;
				case VehicleState::Stopped:
				case VehicleState::Undefined:
				case VehicleState::Count:
					break;
			}*/
		}

		void update_agent(AgentData& agent, TrafficSimulationSystem& system) {
			if (agent.currentGoal == nullptr) {
				return;
			}
			process_vehicle_state(agent, system);
		}
	} // namespace movement_details

} // namespace tjs::core::simulation
