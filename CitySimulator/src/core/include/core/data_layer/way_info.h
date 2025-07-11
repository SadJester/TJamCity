#pragma once

#include <core/simulation_constants.h>
#include <core/data_layer/edge.h>

namespace tjs::core {
	struct Node;
	struct Coordinates;

	TurnDirection get_relative_direction(const Coordinates& a, const Coordinates& o, const Coordinates& b, bool rhs = true);

	struct WayInfo {
		uint64_t uid;
		int lanes;         // Total number of lanes
		int lanesForward;  // Number of lanes in forward direction
		int lanesBackward; // Number of lanes in backward direction
		bool isOneway;     // Whether the way is one-way
		double laneWidth;  // Width of a single lane in meters
		int maxSpeed;
		int layer;
		WayType type;
		WayTag tags;
		std::vector<uint64_t> nodeRefs;
		std::vector<Node*> nodes;
		std::vector<TurnDirection> forwardTurns;
		std::vector<TurnDirection> backwardTurns;

		// Edges that are built after parsing wayInfo and added here for better data access
		std::vector<EdgeHandler> edges;

		static std::unique_ptr<WayInfo> create(uint64_t uid, int lanes, int maxSpeed, WayType type, WayTag tags, int layer = 0) {
			auto way = std::make_unique<WayInfo>();
			way->uid = uid;
			way->lanes = lanes;
			way->lanesForward = lanes;                        // By default, all lanes are forward
			way->lanesBackward = 0;                           // By default, no backward lanes
			way->isOneway = false;                            // By default, ways are bidirectional
			way->laneWidth = SimulationConstants::LANE_WIDTH; // default lane width in meters
			way->maxSpeed = maxSpeed;
			way->type = type;
			way->tags = tags;
			way->layer = layer;
			return way;
		}

		bool is_car_accessible() const;
	};

} // namespace tjs::core
