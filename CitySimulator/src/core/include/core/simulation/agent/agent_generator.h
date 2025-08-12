#pragma once

#include <core/data_layer/vehicle.h>

namespace tjs::core::simulation {
	class VehicleSystem;
	class TrafficSimulationSystem;

	// TODO: Rename to IAgentsGenerator
	class IAgentGenerator {
	public:
		enum class State {
			Undefined,
			InProgress,
			Completed,
			Error
		};

	public:
		IAgentGenerator(TrafficSimulationSystem& system)
			: _system(system) {
		}

		virtual ~IAgentGenerator() = default;

		TrafficSimulationSystem& system() {
			return _system;
		}

		virtual void start_populating() = 0;
		virtual size_t populate() = 0;
		virtual bool is_done() const = 0;
		State get_state() const {
			return _state;
		}

	protected:
		TrafficSimulationSystem& _system;
		State _state = State::Undefined;
	};

} // namespace tjs::core::simulation
