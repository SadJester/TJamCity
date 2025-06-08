#pragma once

#include <nlohmann/json.hpp>

#include <core/simulation_constants.h>
#include <visualization/visualization_constants.h>

namespace tjs::settings {

	struct MapSettings {
		float selectionDistance = visualization::Constants::DEFAULT_SELECTION_DISTANCE;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MapSettings, selectionDistance)
	};

	struct RenderSettings {
		static constexpr float DEFAULT_FPS = 60.0f;

		int screenWidth = 1280;
		int screenHeight = 720;
		float targetFPS = DEFAULT_FPS;
		float vehicleScaler = core::SimulationConstants::VEHICLE_SCALER;
		MapSettings map;

		static constexpr const char* NAME = "render";

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RenderSettings,
			screenWidth,
			screenHeight,
			targetFPS,
			vehicleScaler,
			map)
	};

} // namespace tjs::settings
