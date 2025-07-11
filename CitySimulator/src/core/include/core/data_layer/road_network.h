#pragma once

#include <core/data_layer/edge.h>

namespace tjs::core {
	// [DON`t USE IT NOW] CH data structures
	struct Edge_Contract {
		uint64_t target;
		double weight;
		bool is_shortcut;
		uint64_t shortcut_id1;
		uint64_t shortcut_id2;

		Edge_Contract(uint64_t t, double w, bool sc = false, uint64_t s1 = 0, uint64_t s2 = 0)
			: target(t)
			, weight(w)
			, is_shortcut(sc)
			, shortcut_id1(s1)
			, shortcut_id2(s2) {}
	};

	struct WayInfo;
	struct Node;

	struct RoadNetwork {
		// List of structures for easier access
		std::unordered_map<uint64_t, Node*> nodes;
		std::unordered_map<uint64_t, WayInfo*> ways;

		std::vector<Edge> edges;
		std::unordered_map<Node*, std::vector<Edge*>> edge_graph;

		// lane connectors
		std::vector<LaneLink> lane_links;
		std::unordered_map<Lane*, std::vector<LaneLinkHandler>> lane_graph;

		// Trivial network for A* without considering lanes
		std::unordered_map<Node*, std::vector<std::pair<Node*, double>>> adjacency_list;
	};

} // namespace tjs::core
