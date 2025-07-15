#pragma once
#include <core/enum_flags.h>
#include <core/simulation_constants.h>

#include <core/data_layer/way_info.h>
#include <core/data_layer/node.h>
#include <core/data_layer/road_network.h>
#include <common/spatial/spatial_grid.h>

namespace tjs::core {
	enum class VehicleType : char {
		SimpleCar,
		SmallTruck,
		BigTruck,
		Ambulance,
		PoliceCar,
		FireTrack,

		Count
	};

	ENUM(MovementError, char,
		None,
		NoOutgoingConnections,
		NoPath,
		NoNextLane);

	ENUM(VehicleState, uint8_t,
		Undefined, PendingMove, Moving, Stopped)

	struct Vehicle {
		uint64_t uid;
		float currentSpeed;
		float maxSpeed;
		Coordinates coordinates;
		VehicleType type;
		WayInfo* currentWay;
		int currentSegmentIndex;
		float rotationAngle; // orientation in radians
		const Lane* current_lane;
		double s_on_lane;
		double lateral_offset;
		VehicleState state;
		MovementError error;
	};
	static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

	using SpatialGrid = tjs::common::SpatialGrid<WayInfo, Lane>;

	struct SegmentBoundingBox {
		Coordinates left;
		Coordinates right;
		Coordinates top;
		Coordinates bottom;
	};

	struct WorldSegment {
		SegmentBoundingBox boundingBox;
		std::unordered_map<uint64_t, std::unique_ptr<Node>> nodes;
		std::unordered_map<uint64_t, std::unique_ptr<WayInfo>> ways;
		std::unordered_map<uint64_t, std::unique_ptr<Junction>> junctions;

		// Sorted by layer
		std::vector<WayInfo*> sorted_ways;

		std::unique_ptr<RoadNetwork> road_network;
		SpatialGrid spatialGrid;

		static std::unique_ptr<WorldSegment> create() {
			auto segment = std::make_unique<WorldSegment>();
			segment->road_network = std::make_unique<RoadNetwork>();
			return segment;
		}

		void rebuild_grid();
	};

	void add_way(SpatialGrid& grid, WayInfo* way);
} // namespace tjs::core
