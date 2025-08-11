#pragma once

#include <nlohmann/json.hpp>

#include <core/simulation/simulation_debug.h>
#include <core/simulation/simulation_types.h>
#include <core/simulation/movement/movement_algorithm.h>

namespace tjs::core {
	enum class AgentGoalSelectionType : char;
} // namespace tjs::core

namespace tjs::core::simulation {
	ENUM(GeneratorType, char, Bulk, Flow);

	struct AgentTask {
		int lane_id = 0;
		int vehicles_per_hour = 100;
		uint64_t goal_node_id = 0;
		int max_vehicles = 0;
		AgentGoalSelectionType goal_selection_type = AgentGoalSelectionType::RandomSelection;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(
			AgentTask,
			lane_id,
			vehicles_per_hour,
			goal_node_id,
			max_vehicles,
			goal_selection_type);
	};
} // namespace tjs::core::simulation

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
		simulation::MovementAlgoType movement_algo = simulation::MovementAlgoType::IDM;
		simulation::GeneratorType generator_type = simulation::GeneratorType::Bulk;
		std::vector<simulation::AgentTask> spawn_requests;

		simulation::SimulationDebugData debug_data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(
			SimulationSettings,
			randomSeed,
			seedValue,
			vehiclesCount,
			steps_on_update,
			step_delta_sec,
			simulation_paused,
			movement_algo,
			debug_data,
			generator_type,
			spawn_requests);
	};

} // namespace tjs::core
