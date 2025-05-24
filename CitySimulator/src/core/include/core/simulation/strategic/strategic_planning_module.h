#pragma once
#include <core/simulation/agent/agent_data.h>

namespace tjs::simulation {
	class TrafficSimulationSystem;

	class StrategicPlanningModule {
	public:
		StrategicPlanningModule(TrafficSimulationSystem& system);
		void update();

	private:
		void updateAgentStrategy(AgentData& agent);

	private:
		TrafficSimulationSystem& _system;
	};
} // namespace tjs::simulation
