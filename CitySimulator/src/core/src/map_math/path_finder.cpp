#include <core/stdafx.h>

#include <core/map_math/path_finder.h>

#include <core/data_layer/node.h>
#include <core/data_layer/data_types.h>

#include <core/map_math/earth_math.h>

namespace tjs::core::algo {

	std::deque<Node*> PathFinder::find_path(const RoadNetwork& network, uint64_t source, uint64_t target) {
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

	std::deque<Node*> PathFinder::find_path_a_star(const RoadNetwork& network, Node* source, Node* target) {
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

	std::unordered_set<Node*> PathFinder::reachable_nodes(const RoadNetwork& network, Node* source) {
		std::unordered_set<Node*> visited;
		if (!source) {
			return visited;
		}

		std::queue<Node*> queue;
		queue.push(source);
		visited.insert(source);

		while (!queue.empty()) {
			Node* current = queue.front();
			queue.pop();

			auto it = network.adjacency_list.find(current);
			if (it == network.adjacency_list.end()) {
				continue;
			}

			for (const auto& [neighbor, cost] : it->second) {
				if (!visited.contains(neighbor)) {
					visited.insert(neighbor);
					queue.push(neighbor);
				}
			}
		}

		return visited;
	}

	bool PathFinder::can_traverse_shortcut(
		const RoadNetwork& network,
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

} // namespace tjs::core::algo
