#pragma once

#define ENUM_FLAG(EnumName, ...) \
    enum class EnumName { __VA_ARGS__, Count }; \
    inline EnumName operator|(EnumName a, EnumName b) { \
        return static_cast<EnumName>(static_cast<int>(a) | static_cast<int>(b)); \
    } \
    inline EnumName operator&(EnumName a, EnumName b) { \
        return static_cast<EnumName>(static_cast<int>(a) & static_cast<int>(b)); \
    } \
    inline bool hasFlag(EnumName a, EnumName b) { \
        return (static_cast<int>(a) & static_cast<int>(b)) != 0; \
    }



namespace tjs::core {
    namespace structures {
        template <typename NodeType>
        class GraphNode {
        public:
            using Ptr = std::unique_ptr<GraphNode<NodeType>>;
            GraphNode() = default;
            GraphNode(NodeType node) : node(node) {}

            NodeType node;
            std::vector<GraphNode*> neighbors;
        };

        template <typename NodeType, typename GraphDataType>
        class Graph : public GraphNode<GraphDataType> {
        public:
            Graph(GraphDataType graphData) 
                : GraphNode<GraphDataType>(graphData)
            {}

            using _NodeType = typename GraphNode<NodeType>;
            using _GraphDataType = typename GraphDataType;

            std::vector<typename GraphNode<NodeType>::Ptr> nodes;
            GraphNode<NodeType>* _root = nullptr;
        };
    
        template<class T>
        concept HasPosition = 
            requires(T a) {
                a.position;
                a.position.longitude;
                a.position.latitude;
            };

        
        template <typename NodeType>
        requires HasPosition<NodeType> && std::is_pod<NodeType>::value
        class SpatialStructure {
        public:
            using Ptr = std::unique_ptr<SpatialStructure<NodeType>>;
            // TODO: replace to custom allocator and pod-type
            std::vector<Ptr> nodes;

            /*
            NodeType* findNearest(const Position& position, double radius) {
                double doubleRadius = radius * radius;
                for (auto& node : nodes) {
                    const double longitudeDistance = node->position.longitude - position.longitude;
                    const double latitudeDistance = node->position.latitude - position.latitude;
                    const double distance = longitudeDistance*longitudeDistance + latitudeDistance*latitudeDistance;
                    if (distance <= doubleRadius) {
                        return &node->node;
                    }
                }
                return nullptr;
            }

            template <typename CheckNodeFn>
            requires std::invocable<CheckNodeFn, NodeType*> && std::is_convertible<std::invoke_result_t<CheckNodeFn, NodeType*>, bool>::value
            std::vector<NodeType*> findInRadius(const Position& position, double radius, size_t expectedNodes = 10, std::optional<CheckNodeFn&&> checkNodeFn = std::nullopt) {
                double doubleRadius = radius * radius;
                std::vector<NodeType*> result;
                result.reserve(expectedNodes);
                for (auto& node : nodes) {
                    const double longitudeDistance = node->position.longitude - position.longitude;
                    const double latitudeDistance = node->position.latitude - position.latitude;
                    const double distance = longitudeDistance*longitudeDistance + latitudeDistance*latitudeDistance;
                    if (distance <= doubleRadius) {
                        result.push_back(&node->node);
                    }
                }
                return result;
            }
            */
        };

    }

    struct Coordinates {
        double longitude;
        double latitude;
    };


    ENUM_FLAG(NodeTags, None, TrafficLight, StopSign, Crosswalk);
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
        VehicleType type;
        float currentSpeed;
        float maxSpeed;
        Coordinates coordinates;
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
