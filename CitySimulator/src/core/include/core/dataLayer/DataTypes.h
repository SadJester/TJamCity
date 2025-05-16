#pragma once
#include <core/enum_flags.h>

namespace tjs::core {
    struct Coordinates {
        double latitude;
        double longitude;
    };


    ENUM_FLAG(NodeTags, None, TrafficLight, StopSign, Crosswalk, Way);
    ENUM_FLAG(WayTags, None, Motorway, Trunk, Primary, Secondary, Tertiary, Residential, Service);

    struct Node {
        uint64_t uid;
        Coordinates coordinates;
        NodeTags tags;

        static std::unique_ptr<Node> create(uint64_t uid, const Coordinates& coordinates, NodeTags tags) {
            auto node = std::make_unique<Node>();
            node->uid = uid;
            node->coordinates = coordinates;
            node->tags = tags;
            return node;
        }

        bool hasTag(NodeTags tag) const { 
            return hasFlag(tags, tag);
        }
    };
    static_assert(std::is_pod<Node>::value, "Data object expect to be POD");

    struct WayInfo  {
        uint64_t uid;
        int lanes;
        int maxSpeed;
        WayTags tags;
        std::vector<uint64_t> nodeRefs;
        std::vector<Node*> nodes;

        static std::unique_ptr<WayInfo> create(uint64_t uid, int lanes, int maxSpeed, WayTags tags) {
            auto way = std::make_unique<WayInfo>();
            way->uid = uid;
            way->lanes = lanes;
            way->maxSpeed = maxSpeed;
            way->tags = tags;
            return way;
        }
    };

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


    struct RoadNetwork {
        std::unordered_map<uint64_t, Node*> nodes;
        std::unordered_map<uint64_t, WayInfo*> ways;
    };

    struct WorldSegment {
        Coordinates left;
        Coordinates right;
        Coordinates top;
        Coordinates bottom;

        std::unordered_map<uint64_t, std::unique_ptr<Node>> nodes;
        std::unordered_map<uint64_t, std::unique_ptr<WayInfo>> ways;
        std::vector<std::unique_ptr<RoadNetwork>> roads;

        static std::unique_ptr<WorldSegment> create() {
            return std::make_unique<WorldSegment>();
        }
    };
}
