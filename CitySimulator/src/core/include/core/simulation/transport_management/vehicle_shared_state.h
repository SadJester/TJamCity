#pragma once

#include <common/sync/shared_state.h>
#include <core/data_layer/vehicle_definitions.h>

namespace tjs::core {

    struct WayInfo;
	struct Lane;
	struct AgentData;

    struct VehicleState1 {
        uint64_t uid;

        double s_on_lane;
        double lateral_offset;

        Coordinates coordinates;
		Lane* current_lane;
		AgentData* agent;

        float current_speed;
        float rotation_angle;
        float length;
        float width;
        float max_speed;

        VehicleType type;
        uint16_t state;
    };

    using VehicleShared = common::sync::shared_state<VehicleState1, 3>;

} // namespace tjs::core
