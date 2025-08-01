#pragma once
#include "core/data_layer/data_types.h"
#include "core/simulation/agent/agent_data.h"

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	class TacticalPlanningModule {
	public:
		TacticalPlanningModule(TrafficSimulationSystem& system);

		void initialize();
		void release();
		void update();

	private:
		TrafficSimulationSystem& _system;
	};

	namespace simulation_details {
		void update_agent(size_t i, AgentData& agent, TrafficSimulationSystem& system);
	} // namespace simulation_details

} // namespace tjs::core::simulation
