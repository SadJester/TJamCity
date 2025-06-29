#pragma once

#include <core/enum_flags.h>
#include <core/simulation_constants.h>

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

	struct Node;

	ENUM_FLAG(TurnDirection, char,
		None = 0,
		Left = 1 << 0,
		Right = 1 << 1,
		Straight = 1 << 2,
		UTurn = 1 << 3,
		MergeRight = 1 << 4,
		MergeLeft = 1 << 5
	);

	struct WayInfo {
		uint64_t uid;
		int lanes;         // Total number of lanes
		int lanesForward;  // Number of lanes in forward direction
		int lanesBackward; // Number of lanes in backward direction
		bool isOneway;     // Whether the way is one-way
		double laneWidth;  // Width of a single lane in meters
		int maxSpeed;
		WayType type;
		std::vector<uint64_t> nodeRefs;
		std::vector<Node*> nodes;
		std::vector<TurnDirection> forwardTurns;
		std::vector<TurnDirection> backwardTurns;

		static std::unique_ptr<WayInfo> create(uint64_t uid, int lanes, int maxSpeed, WayType type) {
			auto way = std::make_unique<WayInfo>();
			way->uid = uid;
			way->lanes = lanes;
			way->lanesForward = lanes;                        // By default, all lanes are forward
			way->lanesBackward = 0;                           // By default, no backward lanes
			way->isOneway = false;                            // By default, ways are bidirectional
			way->laneWidth = SimulationConstants::LANE_WIDTH; // default lane width in meters
			way->maxSpeed = maxSpeed;
			way->type = type;
			return way;
		}

		bool is_car_accessible() const;
	};

} // namespace tjs::core
