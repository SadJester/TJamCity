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

		g_score[source] = 0.0;
		double h_start = core::algo::euclidean_distance(source->coordinates, target->coordinates);
		open_set.emplace(h_start, source);

		while (!open_set.empty()) {
			Node* current = open_set.top().second;
			open_set.pop();

			if (current == target) {
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
					double h = core::algo::euclidean_distance(neighbor->coordinates, target->coordinates);
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

			auto it = network.edge_graph.find(current);
			if (it == network.edge_graph.end()) {
				continue;
			}

			for (const Edge* edge : it->second) {
				Node* neighbor = edge->end_node;
				if (!visited.contains(neighbor)) {
					visited.insert(neighbor);
					queue.push(neighbor);
				}
			}
		}

		return visited;
	}

	std::vector<const Edge*> PathFinder::find_edge_path_a_star(const RoadNetwork& network, Node* source, Node* target) {
		using NodeEntry = std::pair<double, Node*>;
		std::priority_queue<NodeEntry, std::vector<NodeEntry>, std::greater<>> open_set;

		std::unordered_map<Node*, double> g_score;
		std::unordered_map<Node*, Node*> came_from;
		std::unordered_map<Node*, const Edge*> came_by_edge;
		std::unordered_set<Node*> closed_set;

		g_score[source] = 0.0;
		double h_start = core::algo::euclidean_distance(source->coordinates, target->coordinates);
		open_set.emplace(h_start, source);

		while (!open_set.empty()) {
			Node* current = open_set.top().second;
			open_set.pop();

			if (current == target) {
				std::vector<const Edge*> path;
				while (current != source) {
					const Edge* e = came_by_edge[current];
					path.insert(path.begin(), e);
					current = came_from[current];
				}
				return path;
			}

			if (closed_set.count(current)) {
				continue;
			}
			closed_set.insert(current);

			auto it = network.edge_graph.find(current);
			if (it == network.edge_graph.end()) {
				continue;
			}

			for (const Edge* edge : it->second) {
				Node* neighbor = edge->end_node;
				double tentative_g = g_score[current] + edge->length;

				if (!g_score.count(neighbor) || tentative_g < g_score[neighbor]) {
					came_from[neighbor] = current;
					came_by_edge[neighbor] = edge;
					g_score[neighbor] = tentative_g;
					double h = core::algo::euclidean_distance(neighbor->coordinates, target->coordinates);
					open_set.emplace(tentative_g + h, neighbor);
				}
			}
		}

		return {};
	}

	struct NodeRecord {
		Node* parent = nullptr;
		const Edge* via = nullptr;
		double g_score = std::numeric_limits<double>::infinity();
	};

	// ────────────────────────────────────────────────────────────────────
	//  A* from *lane*            (multi-source front edges)
	//  --------------------------------------------------
	//  • start_lane  : the lane the vehicle is physically in **now**
	//  • target      : destination node
	//  • network     : unchanged RoadNetwork with edge_graph
	//
	//  Returns edge sequence   start_lane → … → target
	//  or empty vector if no route exists.
	// ────────────────────────────────────────────────────────────────────
	std::vector<const Edge*> PathFinder::find_edge_path_a_star_from_lane(
		const RoadNetwork& network,
		const Lane* start_lane,
		Node* target,
		bool look_adjacent_lanes) {
		TJS_TRACY_NAMED("PathFinder::find_edge_path_a_star_from_lane");

		using NodeEntry = std::pair<double, Node*>;
		using OpenSetQueue = std::priority_queue<
			std::pair<double, Node*>,
			std::vector<std::pair<double, Node*>>,
			std::greater<>>;

		static thread_local std::unordered_map<Node*, NodeRecord> records;
		static thread_local std::unordered_set<Node*> closed_set;
		static thread_local OpenSetQueue open_set;

		records.reserve(128);
		closed_set.reserve(128);

		records.clear();
		closed_set.clear();
		open_set = {};

		auto seed_successors = [&](const Lane* ln) {
			for (LaneLinkHandler h : ln->outgoing_connections) {
				const LaneLink& link = *h;
				const Edge* e = link.to ? link.to->parent : nullptr;
				if (!e) {
					continue;
				}

				Node* node = e->end_node;
				NodeRecord rec;
				rec.parent = nullptr;
				rec.via = e;
				rec.g_score = 0.0;
				records[node] = rec;

				double h_cost = core::algo::euclidean_distance(node->coordinates, target->coordinates);
				open_set.emplace(h_cost, node);
			}
		};

		if (look_adjacent_lanes) {
			for (auto& lane : start_lane->parent->lanes) {
				seed_successors(&lane);
			}
		} else {
			seed_successors(start_lane);
		}

		const auto has_transition = [](const Edge& from, const Edge& to) {
			return std::ranges::find(from.outgoing_edges, &to) != from.outgoing_edges.end();
		};

		while (!open_set.empty()) {
			Node* current = open_set.top().second;
			open_set.pop();

			if (current == target) {
				std::vector<const Edge*> path;
				Node* n = current;
				while (records.contains(n)) {
					const auto& rec = records[n];
					if (!rec.via) {
						break; // entry point
					}
					path.insert(path.begin(), rec.via);
					n = rec.parent;
				}
				return path;
			}

			auto result = closed_set.insert(current);
			if (!result.second) {
				continue;
			}

			auto it = network.edge_graph.find(current);
			if (it == network.edge_graph.end()) {
				continue;
			}

			auto from_it = records.find(current);
			const double tentative_base = from_it->second.g_score;

			for (const Edge* edge : it->second) {
				Node* neighbor = edge->end_node;

				// check lane-level connectivity
				if (from_it->second.via) {
					if (!has_transition(*from_it->second.via, *edge)) {
						continue;
					}
				}

				const double tentative_g = tentative_base + edge->length;
				const double h = core::algo::euclidean_distance(neighbor->coordinates, target->coordinates);

				auto it = records.find(neighbor);
				if (it == records.end()) {
					records[neighbor] = NodeRecord { current, edge, tentative_g };
					open_set.emplace(tentative_g + h, neighbor);
				} else if (tentative_g < it->second.g_score) {
					it->second = NodeRecord { current, edge, tentative_g };
					open_set.emplace(tentative_g + h, neighbor);
				}
			}
		}

		return {}; // no path found
	}

} // namespace tjs::core::algo
