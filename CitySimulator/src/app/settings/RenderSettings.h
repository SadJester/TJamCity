#pragma once

#include <nlohmann/json.hpp>

namespace tjs::settings {
    
    struct MapSettings {
        // Colors
    };

    struct RenderSettings {
        int screenWidth = 1280;
        int screenHeight = 720;

        static constexpr const char* NAME = "render";

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(RenderSettings,
            screenWidth,
            screenHeight
        )
    };

} // namespace tjs::settings

