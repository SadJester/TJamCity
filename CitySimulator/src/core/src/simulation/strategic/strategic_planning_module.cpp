#include <core/stdafx.h>

#include <core/simulation/strategic/strategic_planning_module.h>

#include <core/simulation/simulation_system.h>
#include <core/simulation/strategic/finding_goal_algo.h>

#include <core/data_layer/world_data.h>
#include <core/map_math/earth_math.h>

namespace tjs::core::simulation {

	StrategicPlanningModule::StrategicPlanningModule(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void StrategicPlanningModule::initialize() {
	}

	void StrategicPlanningModule::release() {
	}

	void StrategicPlanningModule::update() {
		auto& agents = _system.agents();
		for (size_t i = 0; i < agents.size(); ++i) {
			update_agent_strategy(agents[i]);
		}
	}

	void StrategicPlanningModule::update_agent_strategy(AgentData& agent) {
		if (agent.vehicle == nullptr) {
			return;
		}

		if (agent.currentGoal != nullptr) {
			return;
		}

		auto& worldData = _system.worldData();

		static constexpr double MIN_RADIUS = 0.0036;
		static constexpr double MAX_RADIUS = 0.01;

		auto node = find_random_goal(
			worldData.segments().front()->spatialGrid,
			agent.vehicle->coordinates,
			MIN_RADIUS,
			MAX_RADIUS);

		if (node != nullptr) {
			agent.currentGoal = node;
		}
	}

} // namespace tjs::core::simulation
