#pragma once

#include "render/render_base.h"

namespace tjs::visualization {
	struct Constants {
		static constexpr float LANE_WIDTH = 2.5f;            // meters in world space
		static constexpr double DRAW_LANE_MARKERS_MPP = 1.0; // what meters per pixel should be to draw lane markers

		static constexpr double EARTH_RADIUS = 6378137.0; // in meters
#undef M_PI
		static constexpr double M_PI = 3.14159265358979323846264338327950288;
		static constexpr double DEG_TO_RAD = M_PI / 180.0;

		// Color definitions
		static constexpr FColor ROAD_COLOR = { 0.392f, 0.392f, 0.392f, 1.0f };
		static constexpr FColor LANE_MARKER_COLOR = { 1.0f, 1.0f, 0.0f, 1.0f };
		;
		static constexpr FColor MOTORWAY_COLOR = { 0.314f, 0.314f, 0.471f, 1.0f };
		static constexpr FColor PRIMARY_COLOR = { 0.353f, 0.353f, 0.353f, 1.0f };
		static constexpr FColor RESIDENTIAL_COLOR = { 0.471f, 0.471f, 0.471f, 1.0f };
	};
} // namespace tjs::visualization
