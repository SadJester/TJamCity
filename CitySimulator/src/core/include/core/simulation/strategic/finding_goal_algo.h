#pragma once

#include <common/spatial/spatial_grid.h>

namespace tjs::core {
	struct Node;
	struct Coordinates;
	struct WayInfo;
	struct Lane;
	using SpatialGrid = tjs::common::SpatialGrid<WayInfo, Lane>;
} // namespace tjs::core

namespace tjs::core::simulation {
	Node* find_random_goal(
		const SpatialGrid& grid,
		const Coordinates& coord,
		double minRadius,
		double maxRadius);
} // namespace tjs::core::simulation
