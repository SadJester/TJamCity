#include <core/stdafx.h>

#include <core/data_layer/way_info.h>
#include <core/map_math/earth_math.h>

namespace tjs::core {
	bool WayInfo::is_car_accessible() const {
		return type == WayType::Motorway || type == WayType::Trunk || type == WayType::Primary || type == WayType::Secondary || type == WayType::Tertiary || type == WayType::Residential || type == WayType::Service || type == WayType::MotorwayLink || type == WayType::TrunkLink || type == WayType::PrimaryLink || type == WayType::SecondaryLink || type == WayType::TertiaryLink || type == WayType::Parking;
	}

	TurnDirection get_relative_direction(const Coordinates& a, const Coordinates& o, const Coordinates& b, bool rhs) {
		Coordinates v_in = o - a;  // back-wards so heading points *into* node
		Coordinates v_out = b - o; // usual forward direction
		double θ = algo::signed_angle_deg(v_in, v_out);

		// TODO[simulation_algo] Can be Part_Left, Part_Right also
		if (std::abs(θ) <= 30.0) {
			return TurnDirection::Straight;
		}
		if (θ > 30.0 && θ <= 150.0) {
			return rhs ? TurnDirection::Left : TurnDirection::Right;
		}
		if (θ < -30.0 && θ >= -150.0) {
			return rhs ? TurnDirection::Right : TurnDirection::Left;
		}
		return TurnDirection::UTurn;
	}

} // namespace tjs::core
