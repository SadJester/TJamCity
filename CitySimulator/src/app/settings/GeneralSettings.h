#pragma once

#include <nlohmann/json.hpp>

#include <core/dataLayer/DataTypes.h>

namespace tjs::settings {

    struct GeneralSettings {
        std::string selectedFile;
        //core::Coordinates projectionCenter;
        double zoomLevel;

        int vehiclesCount = 100;

        static constexpr const char* NAME = "General";
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(GeneralSettings,
            selectedFile,
            //projectionCenter,
            zoomLevel,
            vehiclesCount
        )
    };
}
