#pragma once

#include <common/help_fixtures.h>
#include <common/container_ptr_handler.h>

namespace tjs::core {
	struct Edge;
	struct Lane;

	struct LaneLink {
		Lane* from = nullptr;
		Lane* to = nullptr;
		bool yield = false;
	};

	using LaneLinkHandler = common::ContainerPtrHolder<std::vector<LaneLink>>;

	struct Lane : public common::WithId<Lane> {
		Edge* parent = nullptr;
		LaneOrientation orientation = LaneOrientation::Forward;
		double width = 0.0;
		double length = 0.0;
		std::vector<Coordinates> centerLine;
		TurnDirection turn = TurnDirection::None;
		std::vector<LaneLinkHandler> outgoing_connections;
		std::vector<LaneLinkHandler> incoming_connections;
	};
} // namespace tjs::core
