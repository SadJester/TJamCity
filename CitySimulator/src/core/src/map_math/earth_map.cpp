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
		if (std::abs(a.latitude) > 90.0 || std::abs(b.latitude) > 90.0 || std::abs(a.longitude) > 180.0 || std::abs(b.longitude) > 180.0) {
			// TODO: Algo error handling
			throw std::invalid_argument("Invalid coordinates: latitude must be in [-90,90] and longitude in [-180,180]");
		}
		const double lat1 = to_radians(a.latitude);
		const double lon1 = to_radians(a.longitude);
		const double lat2 = to_radians(b.latitude);
		const double lon2 = to_radians(b.longitude);

		const double dlat = lat2 - lat1;
		const double dlon = lon2 - lon1;

		// For very small distances, use simpler approximation
		if (std::abs(dlat) < 1e-10 && std::abs(dlon) < 1e-10) {
			const double x = dlon * cos((lat1 + lat2) / 2);
			const double y = dlat;
			return MathConstants::EARTH_RADIUS * sqrt(x * x + y * y);
		}

		const double a_harv = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
		return MathConstants::EARTH_RADIUS * 2 * atan2(sqrt(a_harv), sqrt(1 - a_harv));
	}

	double bearing(const Coordinates& from, const Coordinates& to) {
		double lat1 = to_radians(from.latitude);
		double lat2 = to_radians(to.latitude);
		double dlon = to_radians(to.longitude - from.longitude);
		double y = sin(dlon) * cos(lat2);
		double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dlon);
		double brng = atan2(y, x);
		return std::fmod(to_degrees(brng) + 360.0, 360.0);
	}

	Coordinates offset_coordinate(
		const Coordinates& origin,
		double heading_degrees,
		double lateral_offset_meters) {
		// Convert inputs to radians
		double heading_rad = to_radians(heading_degrees);
		double offset_angle = heading_rad + MathConstants::M_PI / 2.0; // right side

		double dlat = (lateral_offset_meters * cos(offset_angle)) / MathConstants::EARTH_RADIUS;
		double dlon = (lateral_offset_meters * sin(offset_angle)) / (MathConstants::EARTH_RADIUS * cos(to_radians(origin.latitude)));

		Coordinates result;
		result.latitude = origin.latitude + to_degrees(dlat);
		result.longitude = origin.longitude + to_degrees(dlon);
		return result;
	}
} // namespace tjs::core::algo
