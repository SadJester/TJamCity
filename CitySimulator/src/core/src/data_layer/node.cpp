#include <core/stdafx.h>
#include <core/data_layer/node.h>

namespace tjs::core {

	// Addition operator
	Coordinates operator+(const Coordinates& a, const Coordinates& b) {
		return {
			a.latitude + b.latitude,
			a.longitude + b.longitude,
			a.x + b.x,
			a.y + b.y
		};
	}

	// Subtraction operator
	Coordinates operator-(const Coordinates& a, const Coordinates& b) {
		return {
			a.latitude - b.latitude,
			a.longitude - b.longitude,
			a.x - b.x,
			a.y - b.y
		};
	}

} // namespace tjs::core
