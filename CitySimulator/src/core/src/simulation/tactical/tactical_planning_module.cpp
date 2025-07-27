#include <core/stdafx.h>

#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/simulation_system.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/earth_math.h>

#include <core/map_math/path_finder.h>

namespace tjs::core::simulation {

	TacticalPlanningModule::TacticalPlanningModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void TacticalPlanningModule::initialize() {
	}

	void TacticalPlanningModule::release() {
	}

	void TacticalPlanningModule::update() {
		TJS_TRACY_NAMED("TacticalPlanning_Update");
		auto& agents = _system.agents();
		for (size_t i = 0; i < agents.size(); ++i) {
			simulation_details::update_agent(i, agents[i], _system);
		}
	}

	namespace simulation_details {
		std::vector<Edge*> find_path(Lane* start_lane, Node* goal, RoadNetwork& road_network, bool look_adjacent_lanes) {
			std::vector<Edge*> result;
			auto edge_path = core::algo::PathFinder::find_edge_path_a_star_from_lane(
				road_network,
				start_lane,
				goal,
				look_adjacent_lanes);
			result.reserve(edge_path.size());
			for (const auto* edge : edge_path) {
				result.push_back(const_cast<Edge*>(edge));
			}

			return result;
		}

		Node* find_nearest_node(const Coordinates& coords, RoadNetwork& road_network) {
			Node* nearest = nullptr;
			double min_distance = std::numeric_limits<double>::max();

			for (const auto& [id, node] : road_network.nodes) {
				double dist = core::algo::euclidean_distance(node->coordinates, coords);
				if (dist < min_distance) {
					min_distance = dist;
					nearest = node;
				}
			}

			return nearest;
		}

		void reset_goals(AgentData& agent, bool success, bool mark = true) {
			agent.currentGoal = nullptr;
			agent.path.clear();
			if (!success) {
				agent.goalFailCount++;
				if (agent.goalFailCount >= 5) {
					agent.stucked = true;
				}
			} else {
				agent.goalFailCount = 0;
				agent.stucked = false;
			}
		}

		void update_agent(size_t i, AgentData& agent, TrafficSimulationSystem& system) {
			TJS_TRACY_NAMED("TacticalPlanning::update_agent");
			if (agent.vehicle == nullptr || agent.currentGoal == nullptr) {
				return;
			}

			auto& vehicle = *agent.vehicle;

			auto& world = system.worldData();
			auto& segment = world.segments().front();
			auto& road_network = *segment->road_network;
			auto& spatial_grid = segment->spatialGrid;
			auto& buf = system.vehicle_system().vehicle_buffers();

			// reach goal
			if (vehicle.state == VehicleState::Stopped && vehicle.error == MovementError::NoPath) {
				vehicle.error = MovementError::None;
				if (agent.currentGoal != nullptr) {
					const double distance_to_target = core::algo::euclidean_distance(vehicle.coordinates, agent.currentGoal->coordinates);
					if (distance_to_target > SimulationConstants::ARRIVAL_THRESHOLD) {
						// TODO[simulation]: handle agent not close enough to target
						reset_goals(agent, true);
					}
				}
				reset_goals(agent, true);
				return;
			}

			if (vehicle.state == VehicleState::Stopped && vehicle.error == MovementError::NoOutgoingConnections) {
				reset_goals(agent, true);
				agent.stucked = true;
				buf.flags[i] |= FL_ERROR;
				return;
			}

			if (vehicle.state == VehicleState::Stopped && (vehicle.error == MovementError::IncorrectEdge || vehicle.error == MovementError::IncorrectLane)) {
				// need rebuild path
				agent.path.clear();
				vehicle.error = MovementError::None;
			}

			// new goal
			if (agent.path.empty() && vehicle.state == VehicleState::Stopped) {
				Node* start_node = vehicle.current_lane->parent->start_node;

				Lane* start_lane = vehicle.current_lane;

				Node* goal_node = agent.currentGoal;
				if (start_lane && goal_node) {
					const bool find_adjacent = vehicle.s_on_lane < (vehicle.current_lane->length - 10.0);
					agent.path = find_path(start_lane, goal_node, road_network, find_adjacent);

					if (!agent.path.empty()) {
						Edge* first_edge = agent.path.front();
						agent.path.insert(agent.path.begin(), start_lane->parent);

						agent.path_offset = 0;
						agent.goal_lane_mask = build_goal_mask(*start_lane->parent, *first_edge);

						agent.distanceTraveled = 0.0; // Reset distance for new path
						agent.goalFailCount = 0;
						vehicle.state = VehicleState::PendingMove;
						buf.flags[i] &= ~FL_ERROR;

					} else {
						buf.flags[i] |= FL_ERROR;
						reset_goals(agent, false);
					}
				}
			}
		}

	} // namespace simulation_details

} // namespace tjs::core::simulation
