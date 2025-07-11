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

	double euclidean_distance(const Coordinates& a, const Coordinates& b) {
		double dx = b.x - a.x;
		double dy = b.y - a.y;
		return std::sqrt(dx * dx + dy * dy);
	}

	double bearing(const Coordinates& from, const Coordinates& to) {
		double dx = to.x - from.x;
		double dy = to.y - from.y;
		double brng = atan2(dy, dx);
		double deg = to_degrees(brng);
		return normalize_angle(deg);
	}

	double signed_angle_deg(const Coordinates& v1, const Coordinates& v2) {
		double dot = v1.x * v2.x + v1.y * v2.y;
		double det = v1.x * v2.y - v1.y * v2.x;                    // = |v1|·|v2|·sinθ
		return std::atan2(det, dot) * 180.0 / MathConstants::M_PI; // (-180,180]
	}

	double normalize_angle(double deg) noexcept {
		double x = std::fmod(deg + 180.0, 360.0); // now in (-360, 360]
		if (x < 0) {
			x += 360.0; // --> [0, 360)
		}
		return x - 180.0; // --> [-180, 180)
	}

	Coordinates offset_coordinate(
		const Coordinates& origin,
		double heading_degrees,
		double lateral_offset_meters) {
		double heading_rad = to_radians(heading_degrees);
		double offset_angle = heading_rad + MathConstants::M_PI / 2.0; // right side

		Coordinates result = origin;
		result.x += lateral_offset_meters * cos(offset_angle);
		result.y -= lateral_offset_meters * sin(offset_angle);
		return result;
	}
} // namespace tjs::core::algo
