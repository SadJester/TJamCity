#pragma once

#include <core/simulation/agent/agent_data.h>
#include <common/object_pool.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;
	class IAgentGenerator;

	using AgentPool = tjs::common::ObjectPoolExt<AgentData>;
	using AgentPtr = AgentPool::pooled_ptr;

	class AgentManager {
	public:
		AgentManager(TrafficSimulationSystem& system);
		~AgentManager();

		void initialize();
		void release();
		void update();

		void remove_agent(AgentData& agent);

		IAgentGenerator* get_generator() const {
			return _generator.get();
		}

		const std::vector<AgentData*>& agents() {
			return _agent_pool.objects();
		}

	private:
		void remove_agents();
		void populate_agents();

	private:
		TrafficSimulationSystem& _system;
		AgentPool _agent_pool;

		std::unique_ptr<IAgentGenerator> _generator;
		size_t _creation_ticks = 0;
	};

} // namespace tjs::core::simulation
