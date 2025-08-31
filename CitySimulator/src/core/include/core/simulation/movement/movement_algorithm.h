#pragma once

#include <core/simulation/simulation_types.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	class IMovementAlgorithm {
	public:
		IMovementAlgorithm(MovementAlgoType algo_type, TrafficSimulationSystem& system)
			: _system(system)
			, _algo_type(algo_type) {}
		virtual ~IMovementAlgorithm() = default;

		MovementAlgoType get_algo_type() const {
			return _algo_type;
		}

		virtual void initialize() {}
		virtual void release() {}

		virtual void update() = 0;

	protected:
		TrafficSimulationSystem& _system;

	private:
		MovementAlgoType _algo_type;
	};

} // namespace tjs::core::simulation
