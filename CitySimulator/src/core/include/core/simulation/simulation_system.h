#pragma once
#include <core/simulation/time_module.h>
#include <core/simulation/strategic/strategic_planning_module.h>
#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/movement/vehicle_movement_module.h>

namespace tjs::core {
	class WorldData;
} // namespace tjs::core

namespace tjs::core::model {
	class DataModelStore;
}

namespace tjs::simulation {
	class TimeModule;

	class TrafficSimulationSystem {
	public:
		using Agents = std::vector<AgentData>;

	public:
		TrafficSimulationSystem(core::WorldData& data, core::model::DataModelStore& store);
		~TrafficSimulationSystem();

		void initialize();
		void release();
		void update(double realTimeDelta);

		TimeModule& timeModule() {
			return _timeModule;
		}
		Agents& agents() {
			return _agents;
		}
		core::WorldData& worldData() {
			 return _worldData;
		}

		StrategicPlanningModule& strategicModule() {
			return _strategicModule;
		}
		TacticalPlanningModule& tacticalModule() {
			return _tacticalModule;
		}
		VehicleMovementModule& vehicleMovementModule() {
			return _vehicleMovementModule;
		}

		core::model::DataModelStore& store() {
			return _store;
		}

	private:
		Agents _agents;

		TimeModule _timeModule;
		StrategicPlanningModule _strategicModule;
		TacticalPlanningModule _tacticalModule;
		VehicleMovementModule _vehicleMovementModule;
		core::model::DataModelStore& _store;

		core::WorldData& _worldData;
	};
} // namespace tjs::simulation
