#pragma once

namespace tjs::core {
	struct Coordinates;
	struct AgentData;
} // namespace tjs::core

namespace tjs::core::simulation {

	class TrafficSimulationSystem;

	class VehicleMovementModule {
	public:
		VehicleMovementModule(TrafficSimulationSystem& system);

		void initialize();
		void release();
		void update();

	private:
		void update_movement(AgentData& agent);

	private:
		TrafficSimulationSystem& _system;
	};

	namespace movement_details {
		void update_agent(AgentData& agent, TrafficSimulationSystem& system);
	} // namespace movement_details

} // namespace tjs::core::simulation
