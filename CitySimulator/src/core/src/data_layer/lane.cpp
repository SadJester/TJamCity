#include <core/stdafx.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/edge.h>

namespace tjs::core {
	Lane* Lane::left() const {
		const size_t candidate = index_in_edge + 1;
		return parent->lanes.size() > candidate ? &parent->lanes[candidate] : nullptr;
	}

	Lane* Lane::right() const {
		if (index_in_edge == 0) {
			return nullptr;
		}
		return &parent->lanes[index_in_edge - 1];
	}
} // namespace tjs::core
