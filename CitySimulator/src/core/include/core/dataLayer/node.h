#pragma once
#include <core/enum_flags.h>


namespace tjs::core {
    struct Coordinates {
        double latitude;
        double longitude;
    };


    ENUM_FLAG(NodeTags, None, TrafficLight, StopSign, Crosswalk, Way);

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

}