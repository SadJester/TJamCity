#pragma once

#include <core/simulation/movement/movement_algorithm.h>

namespace tjs::core {
	struct Coordinates;
	struct AgentData;
} // namespace tjs::core

namespace tjs::core::simulation {
	class AgentMovementAlgo : public IMovementAlgorithm {
	public:
		AgentMovementAlgo(TrafficSimulationSystem& system);

		void update() override;
	};

	namespace movement_details {
		void update_agent(size_t i, AgentData& agent, TrafficSimulationSystem& system);
	} // namespace movement_details
} // namespace tjs::core::simulation
