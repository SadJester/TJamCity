#pragma once

namespace tjs::core {
	struct Lane;
} // namespace tjs::core

namespace tjs::core::simulation {
	struct LaneRuntime {
		Lane* static_lane = nullptr;
		double length;
		float max_speed;
		std::vector<std::size_t> idx;
	};
} // namespace tjs::core::simulation
