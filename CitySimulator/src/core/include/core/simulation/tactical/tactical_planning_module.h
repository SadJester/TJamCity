#pragma once
#include "core/data_layer/data_types.h"
#include "core/simulation/agent/agent_data.h"

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	namespace _test {
		class ThreadPool;
	} // namespace _test

	class TacticalPlanningModule {
	public:
		TacticalPlanningModule(TrafficSimulationSystem& system);
		~TacticalPlanningModule();

		void initialize();
		void release();
		void update();

	private:
		TrafficSimulationSystem& _system;
		std::unique_ptr<_test::ThreadPool> _pool;
	};

	namespace simulation_details {
		void update_agent(size_t i, AgentData& agent, TrafficSimulationSystem& system);
	} // namespace simulation_details

} // namespace tjs::core::simulation
