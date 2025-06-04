#include <core/stdafx.h>

#include <core/map_math/contraction_builder.h>

namespace tjs::core::algo {
	void ContractionBuilder::build_graph(core::RoadNetwork& network) {
		// Clear previous data
		network.adjacency_list.clear();

		for (const auto& [way_id, way] : network.ways) {
			const auto& nodes = way->nodes;

			// Connect sequential nodes in the way
			for (size_t i = 0; i < nodes.size() - 1; ++i) {
				Node* current = nodes[i];
				Node* next = nodes[i + 1];

				// Calculate distance between nodes
				double dist = haversine_distance(current->coordinates, next->coordinates);
				//network.adjacency_list[current].emplace_back(next, dist);
				//network.adjacency_list[next].emplace_back(current, dist);

				// Add edges based on way direction and lanes
				if (way->isOneway) {
					// For one-way roads, only add forward direction
					if (way->lanesForward > 0) {
						network.adjacency_list[current].emplace_back(next, dist);
					}
				} else {
					bool added = false;
					// For bidirectional roads, add edges based on lane count
					if (way->lanesForward > 0) {
						network.adjacency_list[current].emplace_back(next, dist);
						added = true;
					}
					if (way->lanesBackward > 0) {
						network.adjacency_list[next].emplace_back(current, dist);
						added = true;
					}
				}
			}
		}
	}

	void ContractionBuilder::build_contraction_hierarchy(core::RoadNetwork& network) {
		compute_node_priorities(network);

		while (!priority_queue.empty()) {
			auto node_id = get_next_node();
			contract_node(network, node_id);
		}
	}

	uint64_t ContractionBuilder::get_next_node() {
		auto node = priority_queue.top();
		priority_queue.pop();
		return node.second;
	}

	int ContractionBuilder::calculate_required_shortcuts(RoadNetwork& network, uint64_t node_id) {
		const auto& upward_edges = network.upward_graph[node_id];
		const auto& downward_edges = network.downward_graph[node_id];

		int shortcuts = 0;
		for (const auto& in_edge : downward_edges) {
			for (const auto& out_edge : upward_edges) {
				if (should_add_shortcut(network, in_edge, out_edge)) {
					shortcuts++;
				}
			}
		}
		return shortcuts;
	}

	void ContractionBuilder::compute_node_priorities(core::RoadNetwork& network) {
		for (const auto& [node_id, node] : network.nodes) {
			double priority = compute_edge_difference(network, node_id);
			priority_queue.emplace(-priority, node_id); // Min-heap
		}
	}

	double ContractionBuilder::compute_edge_difference(core::RoadNetwork& network, uint64_t node_id) {
		// Рассчитываем разницу между добавляемыми shortcuts и удаляемыми ребрами
		int original_edges = network.upward_graph[node_id].size() + network.downward_graph[node_id].size();

		int shortcuts_needed = calculate_required_shortcuts(network, node_id);

		return shortcuts_needed - original_edges;
	}

	void ContractionBuilder::contract_node(core::RoadNetwork& network, uint64_t node_id) {
		auto& edges = network.upward_graph[node_id];
		std::vector<Edge> downward_edges = network.downward_graph[node_id];

		// Обрабатываем входящие и исходящие ребра
		for (const auto& in_edge : downward_edges) {
			for (const auto& out_edge : edges) {
				if (should_add_shortcut(network, in_edge, out_edge)) {
					add_shortcut(network, in_edge, out_edge, node_id);
				}
			}
		}

		// Помечаем узел как обработанный
		network.node_levels[node_id] = network.nodes.size() - priority_queue.size();

		// Удаляем узел из графов
		network.upward_graph.erase(node_id);
		network.downward_graph.erase(node_id);
	}

	bool ContractionBuilder::should_add_shortcut(core::RoadNetwork& network,
		const core::Edge& in_edge,
		const core::Edge& out_edge) {
		// Проверяем существование более короткого пути через другие ребра
		return !has_witness_path(network, in_edge.target, out_edge.target,
			in_edge.weight + out_edge.weight);
	}

	void ContractionBuilder::add_shortcut(core::RoadNetwork& network,
		const core::Edge& in_edge,
		const core::Edge& out_edge,
		uint64_t contracted_node) {
		double weight = in_edge.weight + out_edge.weight;
		uint64_t shortcut_id = network.next_shortcut_id++;

		// Добавляем shortcut в upward graph
		network.upward_graph[in_edge.target].emplace_back(
			out_edge.target,
			weight,
			true,
			in_edge.is_shortcut ? in_edge.shortcut_id1 : in_edge.target,
			out_edge.is_shortcut ? out_edge.shortcut_id2 : out_edge.target);

		// Добавляем обратное ребро в downward graph
		network.downward_graph[out_edge.target].emplace_back(
			in_edge.target,
			weight,
			true,
			in_edge.is_shortcut ? in_edge.shortcut_id1 : in_edge.target,
			out_edge.is_shortcut ? out_edge.shortcut_id2 : out_edge.target);
	}

	bool ContractionBuilder::has_witness_path(core::RoadNetwork& network,
		uint64_t from,
		uint64_t to,
		double limit) {
		// Реализация witness search с использованием Dijkstra или A*
		// Возвращает true, если найден путь короче limit
		// Упрощенная реализация:
		return haversine_distance(network.nodes[from]->coordinates,
				   network.nodes[to]->coordinates)
			   < limit;
	}

} // namespace tjs::core::algo
