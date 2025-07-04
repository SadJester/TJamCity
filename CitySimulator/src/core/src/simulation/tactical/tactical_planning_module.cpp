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
			update_agent_tactics(agents[i]);
		}
	}

	std::vector<Edge*> findEdgePath(Node* start, Node* goal, RoadNetwork& road_network) {
		std::vector<Edge*> result;
		auto edge_path = core::algo::PathFinder::find_edge_path_a_star(road_network, start, goal);

		result.reserve(edge_path.size());
		for (const auto* edge : edge_path) {
			result.push_back(const_cast<Edge*>(edge));
		}

		return result;
	}

	static const Lane* choose_next_lane(const Lane* current_lane, Edge* next_edge) {
		if (current_lane && next_edge) {
			// First try to find a lane that connects directly to the next edge
			for (const LaneLinkHandler& link : current_lane->outgoing_connections) {
				const Lane* l = link->to;
				if (l && l->parent == next_edge) {
					return l;
				}
			}
		}

		// If no direct connection found, return the first lane of the next edge
		if (next_edge && !next_edge->lanes.empty()) {
			return &next_edge->lanes.front();
		}
		return nullptr;
	}

	void TacticalPlanningModule::update_agent_tactics(core::AgentData& agent) {
		TJS_TRACY_NAMED("TacticalPlanning_UpdateAgent");
		using namespace tjs::core;

		if (agent.vehicle == nullptr || agent.currentGoal == nullptr) {
			return;
		}

		auto& vehicle = *agent.vehicle;

		auto& world = _system.worldData();
		auto& segment = world.segments().front();
		auto& road_network = *segment->road_network;
		auto& spatial_grid = segment->spatialGrid;

		// Step 2: If we don't have a path to the goal, find one
		if (!agent.last_segment && agent.path.empty()) {
			Node* start_node = find_nearest_node(vehicle.coordinates, road_network);
			Node* goal_node = agent.currentGoal;

			if (start_node && goal_node) {
				agent.path = findEdgePath(start_node, goal_node, road_network);
				agent.visitedNodes.clear();
				if (!agent.path.empty()) {
					agent.current_goal = agent.path.front();
					agent.target_lane = &agent.current_goal->lanes[0];
					agent.currentStepGoal = agent.current_goal->end_node->coordinates; // agent.target_lane->centerLine.back();
					agent.visitedNodes.push_back(agent.current_goal->start_node);
					agent.path.erase(agent.path.begin());
					agent.distanceTraveled = 0.0; // Reset distance for new path
					agent.goalFailCount = 0;
				} else {
					agent.currentGoal = nullptr;
					agent.current_goal = nullptr;
					agent.target_lane = nullptr;
					agent.last_segment = false;
					agent.goalFailCount++;
					if (agent.goalFailCount >= 5) {
						agent.stucked = true;
					}
					return;
				}
			} else {
				agent.currentGoal = nullptr;
				agent.current_goal = nullptr;
				agent.target_lane = nullptr;
				agent.last_segment = false;
				agent.goalFailCount++;
				if (agent.goalFailCount >= 5) {
					agent.stucked = true;
				}
				return;
			}
		}

		// Step 3: Check if vehicle reached current step goal using haversine distance
		const double distance_to_target = core::algo::euclidean_distance(vehicle.coordinates, agent.currentStepGoal);
		const bool is_expected_lane = vehicle.current_lane != nullptr && vehicle.current_lane->parent == agent.current_goal;
		if (distance_to_target < SimulationConstants::ARRIVAL_THRESHOLD && is_expected_lane) {
			if (!agent.path.empty()) {
				// Update distance traveled with the segment we just completed
				if (!agent.visitedNodes.empty()) {
					auto last_visited = agent.visitedNodes.back();
					agent.distanceTraveled += core::algo::euclidean_distance(last_visited->coordinates, agent.currentStepGoal);
				}

				agent.current_goal = agent.path.front();

				/// Find the best target lane
				Lane* candidate = nullptr;
				double dist = std::numeric_limits<double>::max();
				const auto& pos = agent.vehicle->coordinates;
				for (const LaneLinkHandler& link : agent.vehicle->current_lane->outgoing_connections) {
					Lane* candidate_from_v = link->to;
					for (auto& lane : agent.current_goal->lanes) {
						if (candidate_from_v == &lane) {
							candidate = candidate_from_v;
							break;
						}
					}
				}

				if (candidate == nullptr) {
					// TODO: algo error handling
					// Now just take the first lane and it will be "dancing"
					agent.target_lane = &agent.current_goal->lanes[0];
				} else {
					agent.target_lane = candidate;
				}
				agent.currentStepGoal = agent.target_lane->centerLine.front();
				agent.visitedNodes.push_back(agent.current_goal->start_node);
				agent.path.erase(agent.path.begin());

				agent.last_segment = agent.path.empty();
			} else {
				// Final segment distance
				agent.distanceTraveled += core::algo::euclidean_distance(vehicle.coordinates, agent.currentStepGoal);

				// Reached final destination
				agent.currentGoal = nullptr;
				agent.current_goal = nullptr;
				agent.target_lane = nullptr;
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
		double segLength = core::algo::euclidean_distance(segStart, segEnd);
		if (segLength < 1e-6) { // Very short segment
			return core::algo::euclidean_distance(point, segStart);
		}

		// Calculate projection (simplified for geographic coordinates)
		// Note: This is an approximation as we're working with spherical coordinates
		double u = ((point.x - segStart.x) * (segEnd.x - segStart.x) + (point.y - segStart.y) * (segEnd.y - segStart.y)) / (segLength * segLength);

		u = std::clamp(u, 0.0, 1.0);

		Coordinates projection {
			0.0,
			0.0
		};
		projection.x = segStart.x + u * (segEnd.x - segStart.x);
		projection.y = segStart.y + u * (segEnd.y - segStart.y);

		return core::algo::euclidean_distance(point, projection);
	}

	Node* TacticalPlanningModule::find_nearest_node(const Coordinates& coords, RoadNetwork& road_network) {
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

} // namespace tjs::core::simulation
