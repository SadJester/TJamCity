#pragma once

#include <nlohmann/json.hpp>

namespace tjs::core {

	struct SimulationSettings {
		static constexpr const char* NAME = "simulation_settings";

		static constexpr size_t DEFAULT_VEHICLES_COUNT = 100;

		bool randomSeed = true;
		int seedValue = 0;
		size_t vehiclesCount = DEFAULT_VEHICLES_COUNT;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationSettings,
			randomSeed,
			seedValue,
			vehiclesCount)
	};

} // namespace tjs::core
