#pragma once

#include <core/enum_flags.h>

namespace tjs::core {
	ENUM_FLAG(WayTags, None,
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

	struct Node;

	struct WayInfo {
		uint64_t uid;
		int lanes;         // Total number of lanes
		int lanesForward;  // Number of lanes in forward direction
		int lanesBackward; // Number of lanes in backward direction
		bool isOneway;     // Whether the way is one-way
		int maxSpeed;
		WayTags tags;
		std::vector<uint64_t> nodeRefs;
		std::vector<Node*> nodes;

		static std::unique_ptr<WayInfo> create(uint64_t uid, int lanes, int maxSpeed, WayTags tags) {
			auto way = std::make_unique<WayInfo>();
			way->uid = uid;
			way->lanes = lanes;
			way->lanesForward = lanes; // By default, all lanes are forward
			way->lanesBackward = 0;    // By default, no backward lanes
			way->isOneway = false;     // By default, ways are bidirectional
			way->maxSpeed = maxSpeed;
			way->tags = tags;
			return way;
		}
	};

} // namespace tjs::core
