#pragma once

#include <core/simulation/transport_management/vehicle_state.h>

#include <core/data_layer/vehicle.h>

namespace tjs::core::simulation {
	class TrafficSimulationSystem;

	class VehicleSystem {
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

	private:
		TrafficSimulationSystem& _system;

		VehicleBuffers _buffers;
		std::vector<Vehicle> _vehicles;
	};

} // namespace tjs::core::simulation
