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
			simulation_details::update_agent(agents[i], _system);
		}
	}

	namespace simulation_details {
		std::vector<Edge*> find_path(Node* start, Node* goal, RoadNetwork& road_network) {
			std::vector<Edge*> result;
			auto edge_path = core::algo::PathFinder::find_edge_path_a_star(road_network, start, goal);

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

		void reset_goals(AgentData& agent, bool success) {
			agent.currentGoal = nullptr;
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

		void update_agent(AgentData& agent, TrafficSimulationSystem& system) {
			if (agent.vehicle == nullptr || agent.currentGoal == nullptr) {
				return;
			}

			auto& vehicle = *agent.vehicle;

			auto& world = system.worldData();
			auto& segment = world.segments().front();
			auto& road_network = *segment->road_network;
			auto& spatial_grid = segment->spatialGrid;

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

			// new goal
			if (agent.path.empty() && vehicle.state == VehicleState::Stopped) {
				Node* start_node = find_nearest_node(vehicle.coordinates, road_network);
				Node* goal_node = agent.currentGoal;

				if (start_node && goal_node) {
					agent.path = find_path(start_node, goal_node, road_network);

					if (!agent.path.empty()) {
						agent.distanceTraveled = 0.0; // Reset distance for new path
						agent.goalFailCount = 0;
						vehicle.state = VehicleState::PendingMove;
					} else {
						reset_goals(agent, false);
					}
				}
			}
		}

	} // namespace simulation_details

} // namespace tjs::core::simulation
