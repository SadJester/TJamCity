#pragma once
#include <core/enum_flags.h>
#include <core/simulation_constants.h>

#include <core/dataLayer/way_info.h>
#include <core/dataLayer/node.h>
#include <core/dataLayer/road_network.h>


namespace tjs::core {
    enum class VehicleType : char  {
        SimpleCar,
        SmallTruck,
        BigTruck,
        Ambulance,
        PoliceCar,
        FireTrack
    };

    struct Vehicle {
        int uid;
        float currentSpeed;
        float maxSpeed;
        Coordinates coordinates;
        VehicleType type;
        WayInfo* currentWay;
        int currentSegmentIndex;
        float rotationAngle; // orientation in radians
    };
    static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

    struct Building {
        int uid;
    };
    static_assert(std::is_pod<Building>::value, "Data object expect to be POD");

    struct TrafficLight {
        int uid;
    };
    static_assert(std::is_pod<TrafficLight>::value, "Data object expect to be POD");


    struct PairHash {
        template <typename T1, typename T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            // Combine hashes (boost::hash_combine-like approach)
            return hash1 ^ (hash2 << 1);
        }
    };

    struct SpatialGrid {
        // Spatial grid: Maps grid cell (x, y) to list of WayInfo* in that cell

        using GridKey = std::pair<int, int>;
        using WaysInCell = std::vector<WayInfo*>;
        std::unordered_map<GridKey, WaysInCell, PairHash> spatialGrid;
        double cellSize = SimulationConstants::GRID_CELL_SIZE; // Meters per grid cell (adjust based on road density)

        void add_way(WayInfo* way);

        
        std::optional<std::reference_wrapper<const WaysInCell>> get_ways_in_cell(Coordinates coordinates) const;
        std::optional<std::reference_wrapper<const WaysInCell>> get_ways_in_cell(int x, int y) const;
    };

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
        std::vector<std::unique_ptr<RoadNetwork>> roads;
        SpatialGrid spatialGrid;

        static std::unique_ptr<WorldSegment> create() {
            return std::make_unique<WorldSegment>();
        }

        void rebuild_grid();
    };
}
