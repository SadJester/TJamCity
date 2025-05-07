#pragma once

#include <SDL3/SDL_pixels.h>

namespace tjs::render {
    struct Constants  {
        static constexpr float LANE_WIDTH = 2.5f; // meters in world space
        static constexpr float PIXELS_PER_METER = 2.0f;

        static constexpr double EARTH_RADIUS = 6378137.0; // in meters
        static constexpr double M_PI = 3.1418;
        static constexpr double DEG_TO_RAD = M_PI / 180.0;

        // Color definitions
        static constexpr SDL_FColor ROAD_COLOR = {0.392f, 0.392f, 0.392f, 1.0f};
        static constexpr SDL_FColor LANE_MARKER_COLOR = {0.392f, 0.392f, 0.392f, 1.0f};
        static constexpr SDL_FColor MOTORWAY_COLOR = {0.314f, 0.314f, 0.471f, 1.0f};
        static constexpr SDL_FColor PRIMARY_COLOR = {0.353f, 0.353f, 0.353f, 1.0f};
        static constexpr SDL_FColor RESIDENTIAL_COLOR = {0.471f, 0.471f, 0.471f, 1.0f};
    };
}