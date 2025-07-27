#pragma once

#include <nlohmann/json.hpp>

#include <core/simulation/simulation_debug.h>

namespace tjs::core {
	struct SimulationSettings {
		static constexpr const char* NAME = "simulation_settings";

		static constexpr size_t DEFAULT_VEHICLES_COUNT = 100;
		static constexpr double DEFAULT_FIXED_STEP_SEC = 1.0;
		static constexpr int DEFAULT_STEPS_ON_UPDATE = 10;

		bool randomSeed = true;
		int seedValue = 0;
		size_t vehiclesCount = DEFAULT_VEHICLES_COUNT;
		int steps_on_update = DEFAULT_STEPS_ON_UPDATE;
		double step_delta_sec = DEFAULT_FIXED_STEP_SEC;
		bool simulation_paused = true;

		simulation::SimulationDebugData debug_data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationSettings,
			randomSeed,
			seedValue,
			vehiclesCount,
			steps_on_update,
			step_delta_sec,
			simulation_paused,
			debug_data);
	};

} // namespace tjs::core
