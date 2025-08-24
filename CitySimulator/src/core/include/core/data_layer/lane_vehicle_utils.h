#pragma once

#include <algorithm>

#include <core/data_layer/lane.h>
#include <core/data_layer/data_types.h>

namespace tjs::core {

	inline void insert_vehicle_sorted(Lane& lane, Vehicle* vehicle) {
		auto it = std::lower_bound(lane.vehicles.begin(), lane.vehicles.end(),
			vehicle->s_on_lane,
			[](const Vehicle* v, double s) { return v->s_on_lane > s; });
		lane.vehicles.insert(it, vehicle);
	}

	inline void remove_vehicle(Lane& lane, const Vehicle* vehicle) {
		auto it = std::find(lane.vehicles.begin(), lane.vehicles.end(), vehicle);
		if (it != lane.vehicles.end()) {
			lane.vehicles.erase(it);
		}
	}

	inline void update_vehicle_position(Lane& lane, Vehicle* vehicle) {
		remove_vehicle(lane, vehicle);
		insert_vehicle_sorted(lane, vehicle);
	}

} // namespace tjs::core
