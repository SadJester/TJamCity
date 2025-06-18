#pragma once

namespace tjs::core {

	struct SimulationConstants {
		static constexpr double GRID_CELL_SIZE = 0.0009;
		static constexpr float VEHICLE_SCALER = 10.0f;
		static constexpr double ARRIVAL_THRESHOLD = 2.0;
		static constexpr double LANE_WIDTH = 3.0f;
	};

} // namespace tjs::core
