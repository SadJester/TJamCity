#pragma once
#include "core/data_layer/data_types.h"
#include "core/simulation/agent/agent_data.h"

namespace tjs::simulation {
	class TrafficSimulationSystem;

	class TacticalPlanningModule {
	public:
		TacticalPlanningModule(TrafficSimulationSystem& system);

		void update();

	private:
		void updateAgentTactics(tjs::simulation::AgentData& agent);

	private:
		TrafficSimulationSystem& _system;
	};
} // namespace tjs::simulation
