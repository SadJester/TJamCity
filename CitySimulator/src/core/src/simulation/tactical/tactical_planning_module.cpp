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
		auto& agents = _system.agents();
		for (size_t i = 0; i < agents.size(); ++i) {
			update_agent_tactics(agents[i]);
		}
	}

	std::deque<Node*> findPath(Node* start, Node* goal, RoadNetwork& road_network) {
		return core::algo::PathFinder::find_path_a_star(road_network, start, goal);
	}

	void TacticalPlanningModule::update_agent_tactics(core::AgentData& agent) {
		using namespace tjs::core;

		if (agent.vehicle == nullptr || agent.currentGoal == nullptr) {
			return;
		}

		auto& vehicle = *agent.vehicle;

		auto& world = _system.worldData();
		auto& segment = world.segments().front();
		auto& road_network = *segment->road_network;
		auto& spatial_grid = segment->spatialGrid;

		// if vehicle currentWay is nullptr - find way where vehicle are (std::vector<WayInfo*> ways)

		// find path to agent.currentGoal - save that path to agent`s data
		// 1. set currentStepGoal to path step
		// 2. check if vehicle riched path step
		// * yes: move towards next path point
		// * no: skip
		// 3. if rich destination: nullptr for currentGoal

		// Step 1: If vehicle has no current way, find the nearest way
		if (vehicle.currentWay == nullptr) {
			auto ways_opt = spatial_grid.get_ways_in_cell(vehicle.coordinates);
			if (!ways_opt.has_value()) {
				return;
			}

			const auto& ways = ways_opt->get();
			if (ways.empty()) {
				return;
			}

			WayInfo* closest_way = nullptr;
			double min_distance = std::numeric_limits<double>::max();

			for (WayInfo* way : ways) {
				if (way->nodes.empty()) {
					continue;
				}

				// Use haversine distance
				double dist_first = core::algo::haversine_distance(way->nodes.front()->coordinates, vehicle.coordinates);
				double dist_last = core::algo::haversine_distance(way->nodes.back()->coordinates, vehicle.coordinates);
				double current_min = std::min(dist_first, dist_last);

				if (current_min < min_distance) {
					min_distance = current_min;
					closest_way = way;
				}
			}

			if (closest_way != nullptr) {
				vehicle.currentWay = closest_way;
				vehicle.currentSegmentIndex = find_closest_segmen_index(vehicle.coordinates, closest_way);
			} else {
				return;
			}
		}

		// Step 2: If we don't have a path to the goal, find one
                if (!agent.last_segment && agent.path.empty()) {
                        Node* start_node = find_nearest_node(vehicle.coordinates, road_network);
                        Node* goal_node = agent.currentGoal;

                        if (start_node && goal_node) {
                                agent.path = findPath(start_node, goal_node, road_network);
                                agent.visitedNodes.clear();
                                if (!agent.path.empty()) {
                                        agent.currentStepGoal = agent.path.front()->coordinates;
                                        agent.visitedNodes.push_back(agent.path.front());
                                        agent.path.pop_front();
                                        agent.distanceTraveled = 0.0; // Reset distance for new path
                                        agent.goalFailCount = 0;
                                } else {
                                        agent.currentGoal = nullptr;
                                        agent.last_segment = false;
                                        agent.goalFailCount++;
                                        if (agent.goalFailCount >= 5) {
                                                agent.stucked = true;
                                        }
                                        return;
                                }
                        } else {
                                agent.currentGoal = nullptr;
                                agent.last_segment = false;
                                agent.goalFailCount++;
                                if (agent.goalFailCount >= 5) {
                                        agent.stucked = true;
                                }
                                return;
                        }
                }

		// Step 3: Check if vehicle reached current step goal using haversine distance
		const double distance_to_target = core::algo::haversine_distance(vehicle.coordinates, agent.currentStepGoal);
		if (distance_to_target < SimulationConstants::ARRIVAL_THRESHOLD) {
			if (!agent.path.empty()) {
				// Update distance traveled with the segment we just completed
				if (!agent.visitedNodes.empty()) {
					auto last_visited = agent.visitedNodes.back();
					agent.distanceTraveled += core::algo::haversine_distance(last_visited->coordinates, agent.currentStepGoal);
				}

				// Move to next point in path
				agent.currentStepGoal = agent.path.front()->coordinates;
				agent.visitedNodes.push_back(agent.path.front());
				agent.path.pop_front();
				agent.last_segment = agent.path.empty();
			} else {
				// Final segment distance
				agent.distanceTraveled += core::algo::haversine_distance(vehicle.coordinates, agent.currentStepGoal);

				// Reached final destination
                                agent.currentGoal = nullptr;
                                agent.last_segment = false;
                                agent.goalFailCount = 0;
                                return;
			}
		}
	}

	// Updated helper functions using haversine distance
	int TacticalPlanningModule::find_closest_segmen_index(const Coordinates& coords, WayInfo* way) {
		if (way->nodes.empty()) {
			return -1;
		}

		int closest_index = 0;
		double min_distance = std::numeric_limits<double>::max();

		for (size_t i = 0; i < way->nodes.size() - 1; i++) {
			// Calculate distance to segment using haversine
			double dist = distance_to_segment(coords, way->nodes[i]->coordinates, way->nodes[i + 1]->coordinates);
			if (dist < min_distance) {
				min_distance = dist;
				closest_index = static_cast<int>(i);
			}
		}

		return closest_index;
	}

	// Distance from point to segment (using haversine)
	double TacticalPlanningModule::distance_to_segment(const Coordinates& point,
		const Coordinates& segStart,
		const Coordinates& segEnd) {
		// First check if the point projects onto the segment
		double segLength = core::algo::haversine_distance(segStart, segEnd);
		if (segLength < 1e-6) { // Very short segment
			return core::algo::haversine_distance(point, segStart);
		}

		// Calculate projection (simplified for geographic coordinates)
		// Note: This is an approximation as we're working with spherical coordinates
		double u = ((point.latitude - segStart.latitude) * (segEnd.latitude - segStart.latitude) + (point.longitude - segStart.longitude) * (segEnd.longitude - segStart.longitude)) / (segLength * segLength);

		u = std::clamp(u, 0.0, 1.0);

		Coordinates projection {
			segStart.latitude + u * (segEnd.latitude - segStart.latitude),
			segStart.longitude + u * (segEnd.longitude - segStart.longitude)
		};

		return core::algo::haversine_distance(point, projection);
	}

	Node* TacticalPlanningModule::find_nearest_node(const Coordinates& coords, RoadNetwork& road_network) {
		Node* nearest = nullptr;
		double min_distance = std::numeric_limits<double>::max();

		for (const auto& [id, node] : road_network.nodes) {
			double dist = core::algo::haversine_distance(node->coordinates, coords);
			if (dist < min_distance) {
				min_distance = dist;
				nearest = node;
			}
		}

		return nearest;
	}

} // namespace tjs::core::simulation
