#include <core/stdafx.h>

#include <core/data_layer/vehicle.h>

namespace tjs::core {

	bool Vehicle::is_merging(const Lane& lane) const {
		if (lane_target == nullptr) {
			return false;
		}

		using namespace simulation;
		const uint16_t change_state = static_cast<int>(VehicleStateBits::ST_PREPARE) | static_cast<int>(VehicleStateBits::ST_CROSS);
		return &lane != current_lane && VehicleStateBitsV::has_any(state, change_state, VehicleStateBitsDivision::STATE);
	}

} // namespace tjs::core
