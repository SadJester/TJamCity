#pragma once

#include <core/data_layer/way_info.h>
#include <core/data_layer/node.h>

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

	struct Edge;

	enum class LaneOrientation : char { Forward,
		Backward };

	struct Lane {
		Edge* parent;
		LaneOrientation orientation;
		double width;
		double length;
		std::vector<Coordinates> centerLine;
		TurnDirection turn;
		std::vector<Lane*> outgoing_connections;
		std::vector<Lane*> incoming_connections;

		int id;
		Lane() {
			static int __id = 0;
			this->id = __id++;
		}
		Lane(Lane&&) = default;
		Lane& operator=(Lane&&) = default;
		Lane(const Lane&) = default;
		Lane& operator=(const Lane&) = default;
		~Lane() = default;
	};

	struct Edge {
		std::vector<Lane> lanes;

		core::Node* start_node;
		core::Node* end_node;
		WayInfo* way;
		LaneOrientation orientation;
		double length;

	public:
		Edge() = default;
	};

	struct RoadNetwork {
		// List of structures for easier access
		std::unordered_map<uint64_t, Node*> nodes;
		std::unordered_map<uint64_t, WayInfo*> ways;

		std::vector<Edge> edges;
		std::unordered_map<Node*, std::vector<Edge*>> edge_graph;

		// network of edges that consider not only edges but also lanes
		// consider Junction has lane connectors

		// Trivial network for A* without considering lanes
		std::unordered_map<Node*, std::vector<std::pair<Node*, double>>> adjacency_list;
	};

} // namespace tjs::core
