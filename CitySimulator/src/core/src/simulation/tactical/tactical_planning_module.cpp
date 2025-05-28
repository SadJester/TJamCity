#include <core/stdafx.h>

#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/simulation_system.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/map_math/earth_math.h>

namespace tjs::simulation {
	using namespace tjs::core;

	TacticalPlanningModule::TacticalPlanningModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void TacticalPlanningModule::update() {
		auto& agents = _system.agents();
		for (size_t i = 0; i < agents.size(); ++i) {
			updateAgentTactics(agents[i]);
		}
	}

	class PathFinder {
	public:
		std::deque<Node*> find_path(const RoadNetwork& network, uint64_t source, uint64_t target) {
			if (source == target) {
				return { network.nodes.at(source) };
			}

			std::unordered_set<uint64_t> visited;
			std::queue<std::pair<uint64_t, std::deque<Node*>>> queue;
			queue.emplace(std::make_pair(source, std::deque<Node*> { network.nodes.at(source) }));

			while (!queue.empty()) {
				auto [current, path] = queue.front();
				queue.pop();

				if (current == target) {
					return path;
				}

				if (visited.count(current)) {
					continue;
				}
				visited.insert(current);

				// Проверяем upward-ребра
				for (const auto& edge : network.upward_graph.at(current)) {
					if (edge.weight < std::numeric_limits<double>::infinity() && network.node_levels.at(edge.target) > network.node_levels.at(current)) {
						std::deque<Node*> new_path = path;
						new_path.push_back(network.nodes.at(edge.target));
						queue.emplace(edge.target, new_path);
					}
				}

				// Проверяем downward-ребра
				for (const auto& edge : network.downward_graph.at(current)) {
					if (edge.weight < std::numeric_limits<double>::infinity() && network.node_levels.at(edge.target) < network.node_levels.at(current)) {
						std::deque<Node*> new_path = path;
						new_path.push_back(network.nodes.at(edge.target));
						queue.emplace(edge.target, new_path);
					}
				}
			}

			return {}; // Путь не найден
		}

		std::deque<Node*> find_path_a_star(const RoadNetwork& network, Node* source, Node* target) {
			using NodeEntry = std::pair<double, Node*>;
			std::priority_queue<NodeEntry, std::vector<NodeEntry>, std::greater<>> open_set;

			std::unordered_map<Node*, double> g_score;
			std::unordered_map<Node*, Node*> came_from;
			std::unordered_set<Node*> closed_set;

			// Инициализация
			g_score[source] = 0.0;
			double h_start = core::algo::haversine_distance(source->coordinates, target->coordinates);
			open_set.emplace(h_start, source);

			while (!open_set.empty()) {
				Node* current = open_set.top().second;
				open_set.pop();

				if (current == target) {
					// Восстанавливаем путь
					std::deque<Node*> path;
					while (current != nullptr) {
						path.push_front(current);
						current = came_from[current];
					}
					return path;
				}

				if (closed_set.count(current)) {
					continue;
				}
				closed_set.insert(current);

				const auto& neighbors = network.adjacency_list.find(current);
				if (neighbors == network.adjacency_list.end()) {
					continue;
				}

				for (const auto& [neighbor, edge_cost] : neighbors->second) {
					if (closed_set.count(neighbor)) {
						continue;
					}

					double tentative_g = g_score[current] + edge_cost;

					if (!g_score.count(neighbor) || tentative_g < g_score[neighbor]) {
						came_from[neighbor] = current;
						g_score[neighbor] = tentative_g;
						double h = core::algo::haversine_distance(neighbor->coordinates, target->coordinates);
						open_set.emplace(tentative_g + h, neighbor);
					}
				}
			}

			return {}; // Путь не найден
		}

	private:
		// Вспомогательная функция для проверки возможности перехода через shortcut
		bool can_traverse_shortcut(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge& shortcut) {
			// 1. Проверка на существование shortcut-ребра
			if (!shortcut.is_shortcut) {
				return false;
			}

			// 2. Проверка на корректность идентификаторов shortcut
			if (shortcut.shortcut_id1 == 0 || shortcut.shortcut_id2 == 0) {
				return false;
			}

			// 3. Проверка на обход через более высокие уровни
			if (network.node_levels.at(from) < network.node_levels.at(shortcut.shortcut_id1) || network.node_levels.at(to) < network.node_levels.at(shortcut.shortcut_id2)) {
				return false;
			}

			// 4. Проверка на корректность геометрии
			if (!is_geometry_valid(network, from, to, shortcut)) {
				return false;
			}

			// 5. Проверка на временные ограничения (если применимо)
			if (!is_time_valid(network, from, to, shortcut)) {
				return false;
			}

			// 6. Проверка на транспортные ограничения
			if (!is_transport_valid(network, from, to, shortcut)) {
				return false;
			}

			// 7. Проверка на физические препятствия
			if (!is_obstacle_free(network, from, to, shortcut)) {
				return false;
			}

			return true;
		}

	private:
		// Вспомогательные функции для проверок
		bool is_geometry_valid(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge& shortcut) {
			// Проверка на корректность геометрии shortcut-ребра
			// Можно добавить проверку на пересечение с другими объектами
			// Можно добавить проверку на прямолинейность
			return true;
		}

		bool is_time_valid(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge& shortcut) {
			// Проверка временных ограничений (если они есть)
			// Например, проверка на время суток для определенных дорог
			return true;
		}

		bool is_transport_valid(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge& shortcut) {
			// Проверка транспортных ограничений
			// Например, проверка на допустимый тип транспорта
			return true;
		}

		bool is_obstacle_free(const RoadNetwork& network,
			uint64_t from, uint64_t to,
			const Edge& shortcut) {
			// Проверка на отсутствие физических препятствий
			// Например, проверка на наличие закрытых дорог
			return true;
		}
	};

	std::deque<Node*> findPath(Node* start, Node* goal, RoadNetwork& road_network) {
		PathFinder finder;
		return finder.find_path_a_star(road_network, start, goal);
	}

	void TacticalPlanningModule::updateAgentTactics(tjs::simulation::AgentData& agent) {
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
				vehicle.currentSegmentIndex = findClosestSegmentIndex(vehicle.coordinates, closest_way);
			} else {
				return;
			}
		}

		// Step 2: If we don't have a path to the goal, find one
		if (!agent.last_segment && agent.path.empty()) {
			Node* start_node = findNearestNode(vehicle.coordinates, road_network);
			Node* goal_node = agent.currentGoal;

			if (start_node && goal_node) {
				agent.path = findPath(start_node, goal_node, road_network);
				agent.visitedNodes.clear();
				if (!agent.path.empty()) {
					agent.currentStepGoal = agent.path.front()->coordinates;
					agent.visitedNodes.push_back(agent.path.front());
					agent.path.pop_front();
					agent.distanceTraveled = 0.0; // Reset distance for new path
				}
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
				return;
			}
		}
	}

	// Updated helper functions using haversine distance
	int TacticalPlanningModule::findClosestSegmentIndex(const Coordinates& coords, WayInfo* way) {
		if (way->nodes.empty()) {
			return -1;
		}

		int closest_index = 0;
		double min_distance = std::numeric_limits<double>::max();

		for (size_t i = 0; i < way->nodes.size() - 1; i++) {
			// Calculate distance to segment using haversine
			double dist = distanceToSegment(coords, way->nodes[i]->coordinates, way->nodes[i + 1]->coordinates);
			if (dist < min_distance) {
				min_distance = dist;
				closest_index = static_cast<int>(i);
			}
		}

		return closest_index;
	}

	// Distance from point to segment (using haversine)
	double TacticalPlanningModule::distanceToSegment(const Coordinates& point,
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

	Node* TacticalPlanningModule::findNearestNode(const Coordinates& coords, RoadNetwork& road_network) {
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

} // namespace tjs::simulation
