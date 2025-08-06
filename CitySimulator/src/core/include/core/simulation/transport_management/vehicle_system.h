#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

#include <core/data_layer/vehicle.h>
#include <core/simulation/movement/idm/lane_agnostic_movement.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;
	class ITransportGenerator;
	class IGeneratorListener;

	struct VehicleConfig {
		VehicleType type;
		float length;
		float width;
	};

	class VehicleSystem {
	public:
		using VehicleConfigs = std::unordered_map<VehicleType, VehicleConfig>;
		enum class CreationState {
			InProgress,
			Completed,
			Error
		};

	public:
		explicit VehicleSystem(TrafficSimulationSystem& system);
		virtual ~VehicleSystem();

		void initialize();
		void release();

		// TODO[simulation]: here must be some profile
		size_t populate();
		size_t update();

		CreationState creation_state() const noexcept {
			return _creation_state;
		}

		VehicleBuffers& vehicle_buffers() {
			return _buffers;
		}

		Vehicles& vehicles() {
			return _vehicles;
		}

		const VehicleConfigs& vehicle_configs() const {
			return _vehicle_configs;
		}

		void commit();

		void create_vehicle();

		void add_listener();
		void remove_listener();

	private:
		TrafficSimulationSystem& _system;

		VehicleConfigs _vehicle_configs;
		VehicleBuffers _buffers;
		Vehicles _vehicles;

		std::unique_ptr<ITransportGenerator> _generator;

		std::vector<LaneRuntime> _lane_runtime;

		CreationState _creation_state = CreationState::InProgress;
		size_t _creation_ticks = 0;

	public:
		std::vector<LaneRuntime>& lane_runtime() {
			return _lane_runtime;
		}
	};

} // namespace tjs::core::simulation
