#pragma once

#include <core/simulation/movement/movement_algorithm.h>

namespace tjs::core::simulation {
	class IDMMovementAlgo : public IMovementAlgorithm {
	public:
		IDMMovementAlgo(TrafficSimulationSystem& system);

		void update() override;
	};
} // namespace tjs::core::simulation
