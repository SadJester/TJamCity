#pragma once

namespace tjs::common {
	struct BoundingBox {
		double min_x;
		double min_y;
		double max_x;
		double max_y;
	};

	inline bool intersect(const BoundingBox& a, const BoundingBox& b) {
		return !(a.max_x < b.min_x || a.min_x > b.max_x || a.max_y < b.min_y || a.min_y > b.max_y);
	}

	inline BoundingBox combine(const BoundingBox& a, const BoundingBox& b) {
		return {
			std::min(a.min_x, b.min_x),
			std::min(a.min_y, b.min_y),
			std::max(a.max_x, b.max_x),
			std::max(a.max_y, b.max_y)
		};
	}

	inline double area(const BoundingBox& b) {
		return (b.max_x - b.min_x) * (b.max_y - b.min_y);
	}
} // namespace tjs::common
