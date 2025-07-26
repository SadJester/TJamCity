#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

#include <core/data_layer/vehicle.h>

#include <core/simulation/movement/lane_agnostic_movement.h>

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
		VehicleSystem(TrafficSimulationSystem& system);

		void initialize();
		void release();

		// TODO: here must be some profile
		void create_vehicles();

		VehicleBuffers& vehicle_buffers() {
			return _buffers;
		}

		std::vector<Vehicle>& vehicles() {
			return _vehicles;
		}

		const VehicleConfigs& vehicle_configs() const {
			return _vehicle_configs;
		}

		void commit();

	private:
		TrafficSimulationSystem& _system;

		VehicleConfigs _vehicle_configs;
		VehicleBuffers _buffers;
		std::vector<Vehicle> _vehicles;

		std::vector<LaneRuntime> _lane_runtime;

	public:
		std::vector<LaneRuntime>& lane_runtime() {
			return _lane_runtime;
		}
	};

} // namespace tjs::core::simulation
