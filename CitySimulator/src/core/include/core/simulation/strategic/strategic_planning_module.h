#pragma once
#include <core/simulation/agent/agent_data.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	class StrategicPlanningModule {
	public:
		StrategicPlanningModule(TrafficSimulationSystem& system);
		void initialize();
		void release();
		void update();

	private:
		void update_agent_strategy(AgentData& agent);

	private:
		TrafficSimulationSystem& _system;
	};
} // namespace tjs::core::simulation
