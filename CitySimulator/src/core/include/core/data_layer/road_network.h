#pragma once

#include <core/data_layer/way_info.h>
#include <core/data_layer/node.h>

namespace tjs::core {
	// CH data structures
	struct Edge {
		uint64_t target;
		double weight;
		bool is_shortcut;
		uint64_t shortcut_id1;
		uint64_t shortcut_id2;

		Edge(uint64_t t, double w, bool sc = false, uint64_t s1 = 0, uint64_t s2 = 0)
			: target(t)
			, weight(w)
			, is_shortcut(sc)
			, shortcut_id1(s1)
			, shortcut_id2(s2) {}
	};

	struct RoadNetwork {
		std::unordered_map<uint64_t, Node*> nodes;
		std::unordered_map<uint64_t, WayInfo*> ways;

		std::unordered_map<Node*, std::vector<std::pair<Node*, double>>> adjacency_list;
		std::unordered_map<Node*, double> heuristic_cache;

		std::unordered_map<uint64_t, std::vector<Edge>> upward_graph;
		std::unordered_map<uint64_t, std::vector<Edge>> downward_graph;
		std::unordered_map<uint64_t, int> node_levels;
		uint64_t next_shortcut_id = 1;
	};

} // namespace tjs::core
