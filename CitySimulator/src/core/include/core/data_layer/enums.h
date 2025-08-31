#pragma once

namespace tjs::core {
	ENUM(WayType, char,
		None = 0,
		// Main road hierarchy
		Motorway, Trunk, Primary, Secondary, Tertiary, Residential, Service,
		// Link roads
		MotorwayLink, TrunkLink, PrimaryLink, SecondaryLink, TertiaryLink,
		// Special roads
		Unclassified, Living_Street, Pedestrian, Track, Path,
		// Access roads
		Footway, Cycleway, Bridleway,
		// Amenities
		Parking,
		// Additional road types
		Steps, Corridor, Platform, Construction, Proposed, Bus_Guideway,
		Raceway, Escape, Emergency_Bay, Rest_Area, Services,
		// Special access roads
		Bus_Stop, Emergency_Access, Delivery_Access);

	ENUM_FLAG(TurnDirection, char,
		None = 0,
		Left = 1 << 0,
		Right = 1 << 1,
		Straight = 1 << 2,
		UTurn = 1 << 3,
		MergeRight = 1 << 4,
		MergeLeft = 1 << 5);

	ENUM_FLAG(WayTag, char,
		None = 0,
		Bridge = 1 << 0,
		Tunnel = 1 << 1,
		Embankment = 1 << 2,
		Cutting = 1 << 3);

	ENUM(LaneOrientation, char,
		Forward,
		Backward);

} // namespace tjs::core
