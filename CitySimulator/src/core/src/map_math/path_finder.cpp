#include <core/stdafx.h>

#include <core/map_math/path_finder.h>

#include <core/data_layer/node.h>
#include <core/data_layer/data_types.h>

#include <core/map_math/earth_math.h>

namespace tjs::core::algo {
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
} // namespace tjs::core::algo
