#include <core/stdafx.h>

#include <core/map_math/earth_math.h>
#include <core/math_constants.h>

namespace tjs::core::algo {
	double to_radians(double degrees) {
		return degrees * MathConstants::DEG_TO_RAD;
	}

	double to_degrees(double radians) {
		return radians * MathConstants::RAD_TO_DEG;
	}

	double haversine_distance(const Coordinates& a, const Coordinates& b) {
		const double lat1 = to_radians(a.latitude);
		const double lon1 = to_radians(a.longitude);
		const double lat2 = to_radians(b.latitude);
		const double lon2 = to_radians(b.longitude);

		const double dlat = lat2 - lat1;
		const double dlon = lon2 - lon1;

		const double a_harv = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
		return MathConstants::EARTH_RADIUS * 2 * atan2(sqrt(a_harv), sqrt(1 - a_harv));
	}
} // namespace tjs::core::algo
