#pragma once
#include <core/simulation/time_module.h>
#include <core/simulation/strategic/strategic_planning_module.h>
#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/movement/vehicle_movement_module.h>

#include <common/message_dispatcher/message_dispatcher.h>

namespace tjs::core {
	class WorldData;
	class TimeModule;

	namespace model {
		class DataModelStore;
	} // namespace model

} // namespace tjs::core

namespace tjs::core::simulation {
	class TrafficSimulationSystem {
	public:
		using Agents = std::vector<AgentData>;

	public:
		TrafficSimulationSystem(core::WorldData& data, core::model::DataModelStore& store);
		~TrafficSimulationSystem();

		void initialize();
		void release();
		void update(double realTimeDelta);
		void step();

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

		common::MessageDispatcher& message_dispatcher() {
			return _message_dispatcher;
		}

	private:
		Agents _agents;

		TimeModule _timeModule;
		StrategicPlanningModule _strategicModule;
		TacticalPlanningModule _tacticalModule;
		VehicleMovementModule _vehicleMovementModule;
		core::model::DataModelStore& _store;

		core::WorldData& _worldData;

		common::MessageDispatcher _message_dispatcher;
	};
} // namespace tjs::core::simulation
