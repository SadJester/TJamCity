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
	std::vector<const Edge*> PathFinder::find_edge_path_a_star_from_lane(const RoadNetwork& network,
		const Lane* start_lane,
		Node* target,
		bool look_adjacent_lanes) {
		using NodeEntry = std::pair<double, Node*>; // (fScore , node)
		std::priority_queue<NodeEntry,
			std::vector<NodeEntry>,
			std::greater<>>
			open_set;

		std::unordered_map<Node*, double> g_score;
		std::unordered_map<Node*, Node*> came_from;
		std::unordered_map<Node*, const Edge*> came_by_edge;
		std::unordered_set<Node*> closed_set;

		/* -------------------------------------------------------------
		1. Seed the open-set with every edge reachable *directly*
			from the current lane’s outgoing connections.
		------------------------------------------------------------- */
		auto seed_successors = [&](const Lane* ln) {
			for (LaneLinkHandler h : ln->outgoing_connections) {
				const LaneLink& link = *h;
				const Edge* e = link.to ? link.to->parent : nullptr;
				if (!e) {
					continue;
				}

				Node* first_node = e->end_node; // node after this edge
				double g = 0.0;                 // cost to reach it
				g_score[first_node] = g;
				came_from[first_node] = nullptr; // marks “first hop”
				came_by_edge[first_node] = e;

				double h_cost = core::algo::euclidean_distance(first_node->coordinates, target->coordinates);
				open_set.emplace(h_cost, first_node);
			}
		};

		if (look_adjacent_lanes) {
			for (auto& lane : start_lane->parent->lanes) {
				seed_successors(&lane);
			}
		} else {
			seed_successors(start_lane);
		}

		auto has_transition = [](const Edge& from, const Edge& to) -> bool {
			for (auto& lane : from.lanes) {
				for (auto& link : lane.outgoing_connections) {
					if (link->to->parent == &to) {
						return true;
					}
				}
			}
			return false;
		};

		/* normal A* loop --------------------------------------------- */
		while (!open_set.empty()) {
			Node* current = open_set.top().second;
			open_set.pop();

			if (current == target) {
				/* reconstruct edge path */
				std::vector<const Edge*> path;
				Node* n = current;
				while (came_by_edge.contains(n)) {
					const Edge* e = came_by_edge[n];
					path.insert(path.begin(), e);
					n = came_from[n]; // may become nullptr
				}
				return path; // already includes first edge out of the lane
			}

			if (closed_set.contains(current)) {
				continue;
			}
			closed_set.insert(current);

			auto it = network.edge_graph.find(current);
			if (it == network.edge_graph.end()) {
				continue;
			}

			for (const Edge* edge : it->second) {
				Node* neighbor = edge->end_node;

				// SKIP if no transition from previous edge to this one
				if (came_by_edge.contains(current)) {
					const Edge* from_edge = came_by_edge[current];
					if (!has_transition(*from_edge, *edge)) {
						continue;
					}
				}

				double tentative_g = g_score[current] + edge->length;

				if (!g_score.contains(neighbor) || tentative_g < g_score[neighbor]) {
					came_from[neighbor] = current;
					came_by_edge[neighbor] = edge;
					g_score[neighbor] = tentative_g;

					double h = core::algo::euclidean_distance(neighbor->coordinates, target->coordinates);
					open_set.emplace(tentative_g + h, neighbor);
				}
			}
		}
		return {}; // unreachable
	}

} // namespace tjs::core::algo
