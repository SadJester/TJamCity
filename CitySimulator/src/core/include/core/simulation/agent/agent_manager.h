#pragma once

#include <core/simulation/agent/agent_data.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;
	class IAgentGenerator;

	using Agents = std::vector<AgentData>;

	class AgentManager {
	public:
		AgentManager(TrafficSimulationSystem& system);
		~AgentManager();

		void initialize();
		void release();
		void update();

		void remove_agent(AgentData& agent);

		Agents& agents() {
			return _agents;
		}

		IAgentGenerator* get_generator() const {
			return _generator.get();
		}

	private:
		void remove_agents();
		void populate_agents();

	private:
		TrafficSimulationSystem& _system;
		Agents _agents;

		std::unique_ptr<IAgentGenerator> _generator;
		size_t _creation_ticks = 0;
	};

} // namespace tjs::core::simulation
