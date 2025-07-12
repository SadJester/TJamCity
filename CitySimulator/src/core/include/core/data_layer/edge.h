#pragma once

#include <core/data_layer/lane.h>
#include <common/help_fixtures.h>

namespace tjs::core {
	struct Node;
	struct WayInfo;

	struct Edge : public common::WithId<Edge> {
		enum OppositeSide {
			None,
			Left,
			Right
		};

		std::vector<Lane> lanes;

		core::Node* start_node;
		core::Node* end_node;
		WayInfo* way;
		LaneOrientation orientation;
		double length;

		//EdgeHandler opposite;
		OppositeSide opposite_side = OppositeSide::None;
	};
	using EdgeHandler = common::ContainerPtrHolder<std::vector<Edge>>;
} // namespace tjs::core
