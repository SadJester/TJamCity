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

	struct EdgePrecomp {
		std::vector<uint32_t> lane_exit_mask;
	};
} // namespace tjs::core::simulation
