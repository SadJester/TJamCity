#pragma once

namespace tjs::core {
	struct Lane;
	struct Vehicle;
} // namespace tjs::core

namespace tjs::core::simulation {
	struct LaneRuntime {
		Lane* static_lane = nullptr;
		double length;
		float max_speed;
		std::vector<Vehicle*> idx;
	};
} // namespace tjs::core::simulation
