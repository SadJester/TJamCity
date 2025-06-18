#pragma once

#include <core/data_layer/data_types.h>

namespace tjs::core::algo {
	double to_radians(double degrees);
	double to_degrees(double radians);

	double haversine_distance(const Coordinates& a, const Coordinates& b);
	double bearing(const Coordinates& from, const Coordinates& to);
} // namespace tjs::core::algo
