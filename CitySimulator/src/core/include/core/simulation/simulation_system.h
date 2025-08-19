#pragma once
#include <core/simulation/time_module.h>
#include <core/simulation/strategic/strategic_planning_module.h>
#include <core/simulation/tactical/tactical_planning_module.h>
#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/simulation/simulation_settings.h>
#include <core/simulation/transport_management/vehicle_system.h>
#include <core/simulation/agent/agent_manager.h>

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
		TrafficSimulationSystem(core::WorldData& data, core::model::DataModelStore& store, SimulationSettings& settings);
		~TrafficSimulationSystem();

		void initialize();
		void release();
		void update(double realTimeDelta);
		void step();

		TimeModule& timeModule() {
			return _timeModule;
		}
		std::vector<AgentData*>& agents() {
			return _agent_manager.agents();
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

		VehicleSystem& vehicle_system() {
			return _vehicle_system;
		}

		core::model::DataModelStore& store() {
			return _store;
		}

		common::MessageDispatcher& message_dispatcher() {
			return _message_dispatcher;
		}

		AgentManager& agent_manager() {
			return _agent_manager;
		}

		SimulationSettings& settings() {
			return _settings;
		}

	private:
		TimeModule _timeModule;
		StrategicPlanningModule _strategicModule;
		TacticalPlanningModule _tacticalModule;
		VehicleMovementModule _vehicleMovementModule;

		AgentManager _agent_manager;
		VehicleSystem _vehicle_system;

		core::model::DataModelStore& _store;
		SimulationSettings& _settings;

		core::WorldData& _worldData;

		common::MessageDispatcher _message_dispatcher;
	};
} // namespace tjs::core::simulation
