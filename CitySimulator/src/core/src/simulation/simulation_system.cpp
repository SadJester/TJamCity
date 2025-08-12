#include "core/stdafx.h"

#include <core/simulation/simulation_system.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/store_models/idata_model.h>
#include <core/random_generator.h>

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
		, _agent_manager(*this)
		, _vehicle_system(*this)
		, _vehicleMovementModule(*this) {
	}

	TrafficSimulationSystem::~TrafficSimulationSystem() {
	}

	void sync_agents(TrafficSimulationSystem::Agents& agents, Vehicles& vehicles) {
		// Update pointers
		for (size_t i = 0; i < agents.size(); ++i) {
			agents[i].vehicle = &vehicles[i];
		}

		// Add new vehicles
		for (size_t i = agents.size(); i < vehicles.size(); ++i) {
			agents.push_back({ vehicles[i].uid, &vehicles[i] });
		}
	}

	void TrafficSimulationSystem::initialize() {
		_timeModule.initialize();

		if (!_settings.randomSeed) {
			RandomGenerator::set_seed(_settings.seedValue);
		}

		// clear lanes from vehicles
		if (!_worldData.segments().empty()) {
			auto& segment = _worldData.segments()[0];
			for (auto& edge : segment->road_network->edges) {
				for (auto& lane : edge.lanes) {
					lane.vehicles.clear();
				}
			}
		}

		_agent_manager.initialize();
		_vehicle_system.initialize();

		_strategicModule.initialize();
		_tacticalModule.initialize();
		_vehicleMovementModule.initialize();

		if (_settings.simulation_paused) {
			_timeModule.pause();
		}

		_message_dispatcher.handle_message(events::SimulationInitialized {}, "simulation");
	}

	void TrafficSimulationSystem::release() {
		_agent_manager.release();
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

		_agent_manager.update();
		_vehicle_system.update();

		_strategicModule.update();
		_tacticalModule.update();
		_vehicleMovementModule.update();
	}

} // namespace tjs::core::simulation
