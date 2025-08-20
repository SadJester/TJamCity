#pragma once

#include <core/data_layer/data_types.h>

namespace tjs::core::algo {
	double to_radians(double degrees);
	double to_degrees(double radians);

	double euclidean_distance(const Coordinates& a, const Coordinates& b);
	double bearing(const Coordinates& from, const Coordinates& to);
	double signed_angle_deg(const Coordinates& v1, const Coordinates& v2);
	// Normalize angle to the [-180,180] range. This is used when
	// comparing headings between edges.
	double normalize_angle(double deg) noexcept;

	// Calculates a new coordinate offset from the original point by a
	// given lateral distance (in meters) relative to a heading. Positive
	// distance offsets to the right of the heading direction.
	Coordinates offset_coordinate(
		const Coordinates& origin,
		double heading_degrees,
		double lateral_offset_meters);

	// Returns if vector (move_p2 - move_p1) is in first or fourth quadrant of axis aligned
	// with (axis_p2 - axis_p1)
	// TODO[math]: Should be rewritten as a part of math module near Coordinates
	bool is_in_first_or_fourth(const Coordinates& axis_p1, const Coordinates& axis_p2,
		const Coordinates& move_p1, const Coordinates& move_p2);
} // namespace tjs::core::algo
