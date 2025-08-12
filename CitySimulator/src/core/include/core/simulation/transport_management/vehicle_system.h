#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

#include <core/data_layer/vehicle.h>
#include <core/simulation/movement/idm/lane_agnostic_movement.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	struct VehicleConfig {
		VehicleType type;
		float length;
		float width;
	};

	class VehicleSystem {
	public:
		using VehicleConfigs = std::unordered_map<VehicleType, VehicleConfig>;

	public:
		explicit VehicleSystem(TrafficSimulationSystem& system);
		virtual ~VehicleSystem();

		void initialize();
		void release();
		void update();

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

	public:
		std::vector<LaneRuntime>& lane_runtime() {
			return _lane_runtime;
		}
	};

} // namespace tjs::core::simulation
