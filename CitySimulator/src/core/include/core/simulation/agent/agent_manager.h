#pragma once

#include <core/simulation/agent/agent_data.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;
	class ITransportGenerator;

	using Agents = std::vector<AgentData>;

	class AgentManager {
	public:
		AgentManager(TrafficSimulationSystem& system);
		~AgentManager();

		void initialize();
		void release();
		void update();

		Agents& agents() {
			return _agents;
		}

	private:
		TrafficSimulationSystem& _system;
		Agents _agents;

		std::unique_ptr<ITransportGenerator> _generator;
		size_t _creation_ticks = 0;
	};

} // namespace tjs::core::simulation
