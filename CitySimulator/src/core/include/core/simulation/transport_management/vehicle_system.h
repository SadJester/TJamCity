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
		void update();

		CreationState creation_state() const noexcept {
			return _creation_state;
		}

		// TODO: temporary for transition. Shouldn`t be skipped on review!
		void set_state(CreationState state) {
			_creation_state = state;
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

		// return handle to vehicle
		std::optional<size_t> create_vehicle(Lane& lane, VehicleType type);
		void remove_vehicle(Vehicle& vehicle);

	private:
		TrafficSimulationSystem& _system;

		VehicleConfigs _vehicle_configs;
		VehicleBuffers _buffers;
		Vehicles _vehicles;

		std::vector<LaneRuntime> _lane_runtime;

		CreationState _creation_state = CreationState::InProgress;
		size_t _creation_ticks = 0;

	public:
		std::vector<LaneRuntime>& lane_runtime() {
			return _lane_runtime;
		}
	};

} // namespace tjs::core::simulation
