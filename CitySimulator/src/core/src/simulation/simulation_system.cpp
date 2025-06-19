#include "core/stdafx.h"

#include <core/simulation/simulation_system.h>

#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/store_models/idata_model.h>
#include <core/store_models/vehicle_analyze_data.h>

#include <core/events/simulation_events.h>

namespace tjs::core::simulation {

	TrafficSimulationSystem::TrafficSimulationSystem(core::WorldData& data, core::model::DataModelStore& store)
		: _worldData(data)
		, _store(store)
		, _strategicModule(*this)
		, _tacticalModule(*this)
		, _vehicleMovementModule(*this) {
	}

	TrafficSimulationSystem::~TrafficSimulationSystem() {
	}

	void TrafficSimulationSystem::initialize() {
		auto& vehicles = _worldData.vehicles();

		_agents.clear();
		_agents.shrink_to_fit();
		_agents.reserve(vehicles.size());
		for (size_t i = 0; i < vehicles.size(); ++i) {
			_agents.push_back({ vehicles[i].uid,
				TacticalBehaviour::Normal,
				nullptr,
				core::Coordinates { 0.0, 0.0 },
				&vehicles[i],
				{},
				nullptr,
				{},
				false,
				0.0,
				false,
				0 });
		}

		_strategicModule.initialize();
		_tacticalModule.initialize();
		_vehicleMovementModule.initialize();

		if (_agents.size() == 1) {
			_store.get_model<core::model::VehicleAnalyzeData>()->agent = &_agents[0];
		}

		_message_dispatcher.handle_message(events::SimulationInitialized {}, "simulation");
	}

	void TrafficSimulationSystem::update(double realTimeDelta) {
		_timeModule.update(realTimeDelta);

		_strategicModule.update();
		_tacticalModule.update();
		_vehicleMovementModule.update();
	}

} // namespace tjs::core::simulation
