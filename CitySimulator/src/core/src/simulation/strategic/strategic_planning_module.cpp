#include <core/stdafx.h>

#include <core/simulation/strategic/strategic_planning_module.h>

#include <core/simulation/simulation_system.h>
#include <core/simulation/strategic/finding_goal_algo.h>

#include <core/data_layer/world_data.h>


namespace tjs::simulation {

	StrategicPlanningModule::StrategicPlanningModule(TrafficSimulationSystem& system)
		: _system(system) {}

	void StrategicPlanningModule::update() {
		auto& agents = _system.agents();
		for (size_t i = 0; i < agents.size(); ++i) {
			updateAgentStrategy(agents[i]);
		}
	}

	void StrategicPlanningModule::updateAgentStrategy(AgentData& agent) {
		if (agent.vehicle == nullptr) {
			return;
		}

		if (agent.currentGoal.latitude != 0.0 || agent.currentGoal.longitude != 0.0) {
			return;
		}

		auto& worldData = _system.worldData();
		
		static constexpr double MIN_RADIUS = 40.0;
		static constexpr double MAX_RADIUS = 100.0;

		auto node = findRandomGoal(
			worldData.segments().front()->spatialGrid,
			agent.vehicle->coordinates,
			MIN_RADIUS,
			MAX_RADIUS
		);

		if (node != nullptr) {
			agent.currentGoal = node->coordinates;
		}
	}

} // namespace tjs::simulation
