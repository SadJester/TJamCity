#pragma once

#include "render/RenderBase.h"

namespace tjs::visualization {
    struct Constants  {
        static constexpr float LANE_WIDTH = 2.5f; // meters in world space
        static constexpr float PIXELS_PER_METER = 2.0f;

        static constexpr double EARTH_RADIUS = 6378137.0; // in meters
        static constexpr double M_PI = 3.1418;
        static constexpr double DEG_TO_RAD = M_PI / 180.0;

        // Color definitions
        static constexpr FColor ROAD_COLOR = {0.392f, 0.392f, 0.392f, 1.0f};
        static constexpr FColor LANE_MARKER_COLOR = {0.392f, 0.392f, 0.392f, 1.0f};
        static constexpr FColor MOTORWAY_COLOR = {0.314f, 0.314f, 0.471f, 1.0f};
        static constexpr FColor PRIMARY_COLOR = {0.353f, 0.353f, 0.353f, 1.0f};
        static constexpr FColor RESIDENTIAL_COLOR = {0.471f, 0.471f, 0.471f, 1.0f};
    };
}