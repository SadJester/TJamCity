#pragma once

#include <visual_system/data/map_renderer_data.h>

namespace tjs::core {
	struct Lane;
} // namespace tjs::core

namespace tjs::visualization {
	struct UpdateRenderParamsCommand {
		std::optional<core::Lane*> selected_lane;
		std::optional<bool> network_only_for_selected;
		std::optional<double> simplified_view_threshold;
		std::optional<core::model::MapRendererLayer> visible_layers;
	};

} // namespace tjs::visualization
