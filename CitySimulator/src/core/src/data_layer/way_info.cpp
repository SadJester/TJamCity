#include <core/stdafx.h>

#include <core/data_layer/way_info.h>

namespace tjs::core {
	bool WayInfo::is_car_accessible() const {
		return type == WayType::Motorway || type == WayType::Trunk || type == WayType::Primary || type == WayType::Secondary || type == WayType::Tertiary || type == WayType::Residential || type == WayType::Service || type == WayType::MotorwayLink || type == WayType::TrunkLink || type == WayType::PrimaryLink || type == WayType::SecondaryLink || type == WayType::TertiaryLink || type == WayType::Parking;
	}
} // namespace tjs::core
