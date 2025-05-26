#pragma once
#include <core/simulation/time_module.h>
#include <core/simulation/strategic/strategic_planning_module.h>
#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/movement/vehicle_movement_module.h>

namespace tjs::core {
	class WorldData;
} // namespace tjs::core

namespace tjs::simulation {
	class TimeModule;

	class TrafficSimulationSystem {
	public:
		using Agents = std::vector<AgentData>;

	public:
		TrafficSimulationSystem(core::WorldData& data);
		~TrafficSimulationSystem();

		void initialize();
		void release();
		void update(double realTimeDelta);

		TimeModule& timeModule();
		Agents& agents() {
			return _agents;
		}
		core::WorldData& worldData() {
			 return _worldData;
		}

	private:
		Agents _agents;

		TimeModule _timeModule;
		StrategicPlanningModule _strategicModule;
		TacticalPlanningModule _tacticalModule;
		VehicleMovementModule _vehicleMovementModule;

		core::WorldData& _worldData;
	};
} // namespace tjs::simulation
