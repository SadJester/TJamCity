#pragma once

#include <nlohmann/json.hpp>

#include <core/simulation_constants.h>

namespace tjs::settings {

	struct MapSettings {
		// Colors
	};

	struct RenderSettings {
		static constexpr float DEFAULT_FPS = 60.0f;

		int screenWidth = 1280;
		int screenHeight = 720;
		float targetFPS = DEFAULT_FPS;
		float vehicleScaler = core::SimulationConstants::VEHICLE_SCALER;

		static constexpr const char* NAME = "render";

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RenderSettings,
			screenWidth,
			screenHeight,
			targetFPS,
			vehicleScaler)
	};

} // namespace tjs::settings
