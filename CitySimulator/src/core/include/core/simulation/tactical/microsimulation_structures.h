#pragma once

#include <core/data_layer/data_types.h>
#include <core/data_layer/way_info.h>
#include <core/data_layer/node.h>


namespace tjs::simulation {

    struct SimulationEdge;


    struct SimulationLane {
        SimulationEdge* parent;
        // Data for the lane
    };

    struct SimulationEdge {
        std::vector<SimulationLane> lanes;
        
        core::Node* start_node;
        core::Node* end_node;

    public:
        // methods for easier access
    };

    struct Junction {
        // Data for junction
    };

    struct RoadNetwork {
        // network of edges that consider not only edges but also lanes
        // consider Junction has lane connectors
    };

}
