#pragma once

#include <core/data_layer/vehicle.h>

namespace tjs::core::simulation {
	class VehicleSystem;
	class TrafficSimulationSystem;

	class IGeneratorListener {
	public:
		virtual ~IGeneratorListener() = default;

		virtual void transport_created(Vehicles& vehicles, size_t from, size_t count) = 0;
		virtual void transport_cleared() = 0;
		virtual void populating_done(Vehicles& vehicles) = 0;
	};

	// TODO: Rename to IAgentsGenerator
	class ITransportGenerator {
	public:
		enum class State {
			Undefined,
			InProgress,
			Completed,
			Error
		};

	public:
		ITransportGenerator(TrafficSimulationSystem& system)
			: _system(system) {
		}

		virtual ~ITransportGenerator() = default;

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
