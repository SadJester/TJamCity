#pragma once

namespace tjs::core::simulation {

	class TrafficSimulationSystem;
	class IMovementAlgorithm;

	class VehicleMovementModule {
	public:
		VehicleMovementModule(TrafficSimulationSystem& system);

		void initialize();
		void release();
		void update();

	private:
		TrafficSimulationSystem& _system;

		std::unique_ptr<IMovementAlgorithm> _algorithm;
	};

} // namespace tjs::core::simulation
