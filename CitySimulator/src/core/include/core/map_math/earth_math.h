#pragma once

#include <core/data_layer/data_types.h>

namespace tjs::core::algo {
	double to_radians(double degrees);
	double to_degrees(double radians);

	double haversine_distance(const Coordinates& a, const Coordinates& b);
} // namespace tjs::core::algo
