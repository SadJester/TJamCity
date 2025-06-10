#pragma once

#include <core/data_layer/data_types.h>
#include <render/render_primitives.h>

namespace tjs::visualization {

	struct NodeRenderInfo {
		core::Node* node = nullptr;
		tjs::Position screenPos {};
		bool selected = false;
	};

	struct WayRenderInfo {
		core::WayInfo* way = nullptr;
		std::vector<tjs::Position> screenPoints;
		std::vector<const NodeRenderInfo*> nodes;
		bool selected = false;
	};

	struct VehicleRenderInfo {
		core::Vehicle* vehicle = nullptr;
		tjs::Position screenPos {};
	};

} // namespace tjs::visualization
