#pragma once

#include <core/data_layer/way_info.h>
#include <core/data_layer/node.h>

#include <common/container_ptr_handler.h>

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
	struct Lane;
	struct LaneLink;

	struct LaneLink {
		Lane* from = nullptr;
		Lane* to = nullptr;
		bool yield = false;
	};

	enum class LaneOrientation : char { Forward,
		Backward };

	using LaneLinkHandler = common::ContainerPtrHolder<std::vector<LaneLink>>;

	template <typename _T>
	struct WithId {
		WithId() {
			this->_id = WithId<_T>::global_id++;
		}
		int get_id() const { 
			return _id;
		}

		static void reset_id() { 
			WithId<_T>::global_id = 0;
		}
	private:
		int _id;
		static inline int global_id = 0;
	};

	struct Lane : public WithId<Lane> {
		Edge* parent = nullptr;
		LaneOrientation orientation = LaneOrientation::Forward;
		double width = 0.0;
		double length = 0.0;
		std::vector<Coordinates> centerLine;
		TurnDirection turn = TurnDirection::None;
		std::vector<LaneLinkHandler> outgoing_connections;
		std::vector<LaneLinkHandler> incoming_connections;
	};

	struct Edge : public WithId<Edge> {
		std::vector<Lane> lanes;

		core::Node* start_node;
		core::Node* end_node;
		WayInfo* way;
		LaneOrientation orientation;
		double length;
	};

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
