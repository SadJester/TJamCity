#pragma once

namespace tjs::core {
	struct Node;
	struct SpatialGrid;
	struct Coordinates;
} // namespace tjs::core

namespace tjs::core::simulation {
	Node* find_random_goal(
		const SpatialGrid& grid,
		const Coordinates& coord,
		double minRadius,
		double maxRadius);
} // namespace tjs::core::simulation
