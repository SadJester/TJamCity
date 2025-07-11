#pragma once

#include <nlohmann/json.hpp>

namespace tjs::core {
	struct Coordinates {
		double latitude;
		double longitude;
		double x;
		double y;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Coordinates,
			latitude,
			longitude,
			x,
			y)
	};

	// Addition operator
	Coordinates operator+(const Coordinates& a, const Coordinates& b);

	// Subtraction operator
	Coordinates operator-(const Coordinates& a, const Coordinates& b);
} // namespace tjs::core
