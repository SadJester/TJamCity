#include "core/stdafx.h"

#include <core/simulation/simulation_system.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/store_models/idata_model.h>
#include <core/store_models/vehicle_analyze_data.h>

#include <core/events/simulation_events.h>

namespace tjs::core::simulation {

	TrafficSimulationSystem::TrafficSimulationSystem(
		core::WorldData& data,
		core::model::DataModelStore& store,
		core::SimulationSettings& settings)
		: _settings(settings)
		, _worldData(data)
		, _store(store)
		, _timeModule(*this)
		, _strategicModule(*this)
		, _tacticalModule(*this)
		, _vehicle_system(*this)
		, _vehicleMovementModule(*this) {
#if TJS_SIMULATION_DEBUG
		_store.create<SimulationDebugData>();
#endif
	}

	TrafficSimulationSystem::~TrafficSimulationSystem() {
	}

	void sync_agents(TrafficSimulationSystem::Agents& agents, VehicleSystem::Vehicles& vehicles) {
		// Update pointers
		for (size_t i = 0; i < agents.size(); ++i) {
			agents[i].vehicle = &vehicles[i];
		}

		// Add new vehicles
		for (size_t i = agents.size(); i < vehicles.size(); ++i) {
			agents.push_back({ vehicles[i].uid,
				TacticalBehaviour::Normal,
				nullptr,
				&vehicles[i],
				{},
				0,
				0,
				0.0f,
				false,
				0 });
		}
	}

	void TrafficSimulationSystem::initialize() {
		_timeModule.initialize();

		_vehicle_system.initialize();
		_vehicle_system.populate();

		_agents.clear();
		_agents.reserve(_settings.vehiclesCount);
		sync_agents(_agents, _vehicle_system.vehicles());

		_strategicModule.initialize();
		_tacticalModule.initialize();
		_vehicleMovementModule.initialize();

		if (_settings.simulation_paused) {
			_timeModule.pause();
		}

		if (_agents.size() == 1) {
			_store.get_entry<core::model::VehicleAnalyzeData>()->agent = &_agents[0];
		}

		_message_dispatcher.handle_message(events::SimulationInitialized {}, "simulation");
	}

	void TrafficSimulationSystem::release() {
		_vehicle_system.release();
		_strategicModule.release();
		_tacticalModule.release();
		_vehicleMovementModule.release();
	}

	void TrafficSimulationSystem::update(double realTimeDelta) {
		_timeModule.update(realTimeDelta);

		if (_timeModule.state().isPaused) {
			return;
		}

		for (int i = 0; i < _settings.steps_on_update; ++i) {
			step();
		}
	}

	void TrafficSimulationSystem::step() {
		_timeModule.tick();

		size_t created = _vehicle_system.update();
		if (created > 0) {
			sync_agents(_agents, _vehicle_system.vehicles());
		}

		_strategicModule.update();
		_tacticalModule.update();
		_vehicleMovementModule.update();
	}

} // namespace tjs::core::simulation
