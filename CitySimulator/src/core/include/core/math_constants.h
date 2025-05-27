#pragma once


namespace tjs::core {
    struct MathConstants {
        static constexpr double EARTH_RADIUS = 6378137.0; // in meters
		static constexpr double M_PI = 3.14159265358979323846264338327950288;
		static constexpr double DEG_TO_RAD = M_PI / 180.0;
        static constexpr double RAD_TO_DEG = 180.0 / M_PI;
    };
} // namespace tjs::core

