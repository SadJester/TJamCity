#include <core/stdafx.h>

#include <core/simulation/strategic/finding_goal_algo.h>

#include <core/data_layer/node.h>
#include <core/data_layer/data_types.h>
#include <core/data_layer/way_info.h>
#include <core/math_constants.h>

namespace tjs::core::simulation {

	core::Node* find_random_goal(
		const core::SpatialGrid& grid,
		const core::Coordinates& coord,
		double minRadius,
		double maxRadius) {
		// Step 1: Find the grid cell for the given coordinate
		auto origin_cell = grid.get_ways_in_cell(coord);
		if (!origin_cell.has_value()) {
			return nullptr;
		}

		// Calculate grid coordinates of the origin cell
		int origin_x = static_cast<int>(coord.latitude / grid.cellSize);
		int origin_y = static_cast<int>(coord.longitude / grid.cellSize);

		// Step 2: Find another random cell within radius range
		static std::random_device rd;
		static std::mt19937 gen(rd());

		// Convert radius to grid cells
		int min_cell_radius = static_cast<int>(minRadius / grid.cellSize);
		int max_cell_radius = static_cast<int>(maxRadius / grid.cellSize);

		if (min_cell_radius >= max_cell_radius) {
			return nullptr; // Invalid radius range
		}

		// We'll try a few times to find a suitable cell
		const int max_attempts = 100;
		for (int attempt = 0; attempt < max_attempts; ++attempt) {
			// Generate random radius and angle
			std::uniform_int_distribution<> radius_dist(min_cell_radius, max_cell_radius);
			std::uniform_real_distribution<> angle_dist(0.0, 2.0 * core::MathConstants::M_PI);

			int radius = radius_dist(gen);
			double angle = angle_dist(gen);

			// Calculate target cell coordinates
			int target_x = origin_x + static_cast<int>(radius * std::cos(angle));
			int target_y = origin_y + static_cast<int>(radius * std::sin(angle));

			// Step 3: Get ways in the target cell and pick a random way's first node
			auto target_cell = grid.get_ways_in_cell(target_x, target_y);
			if (target_cell.has_value() && !target_cell->get().empty()) {
				const auto& ways = target_cell->get();

				// Pick a random way
				std::uniform_int_distribution<> way_dist(0, ways.size() - 1);
				core::WayInfo* random_way = ways[way_dist(gen)];

				// Return the first node of the way
				if (!random_way->nodes.empty()) {
					return random_way->nodes[0];
				}
			}
		}

		return nullptr; // Failed to find a suitable node after max attempts
	}

} // namespace tjs::core::simulation
