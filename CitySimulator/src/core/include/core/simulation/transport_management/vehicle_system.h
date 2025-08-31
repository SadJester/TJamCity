#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

#include <core/data_layer/vehicle.h>
#include <core/simulation/movement/idm/lane_agnostic_movement.h>
#include <common/object_pool.h>

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
		using VehiclePool = tjs::common::ObjectPoolExt<Vehicle>;
		using VehiclePtr = VehiclePool::pooled_ptr;

	public:
		explicit VehicleSystem(TrafficSimulationSystem& system);
		virtual ~VehicleSystem();

		void initialize();
		void release();
		void update();

		const std::vector<Vehicle*>& vehicles() {
			return _vehicle_pool.objects();
		}

		const VehicleConfigs& vehicle_configs() const {
			return _vehicle_configs;
		}

		void commit();

		// return handle to vehicle
		std::optional<Vehicle*> create_vehicle(Lane& lane, VehicleType type, float desired_speed);
		void remove_vehicle(Vehicle* vehicle);

	private:
		TrafficSimulationSystem& _system;

		VehicleConfigs _vehicle_configs;
		VehiclePool _vehicle_pool;

		std::vector<LaneRuntime> _lane_runtime;

	public:
		std::vector<LaneRuntime>& lane_runtime() {
			return _lane_runtime;
		}
	};

} // namespace tjs::core::simulation
