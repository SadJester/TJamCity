#pragma once

namespace tjs::core
{
    
    enum class VehicleType : char {
		SimpleCar,
		SmallTruck,
		BigTruck,
		Ambulance,
		PoliceCar,
		FireTrack,

		Count
	};

	ENUM(VehicleState, uint8_t,
		Undefined, PendingMove, Moving, Stopped);

} // namespace tjs::core
