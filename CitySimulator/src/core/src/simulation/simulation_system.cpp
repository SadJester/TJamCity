#include "core/stdafx.h"

#include <core/simulation/simulation_system.h>

#include <core/dataLayer/data_types.h>
#include <core/dataLayer/WorldData.h>


namespace tjs::simulation {

    TrafficSimulationSystem::TrafficSimulationSystem(core::WorldData& data)
        : _worldData(data)
        , _strategicModule(*this)
        , _tacticalModule(*this) {
    }

    TrafficSimulationSystem::~TrafficSimulationSystem() {
    }

    void TrafficSimulationSystem::initialize() {
        auto& vehicles = _worldData.vehicles();

        _agents.clear();
        _agents.shrink_to_fit();
        _agents.reserve(vehicles.size());
        for (size_t i = 0; i < vehicles.size(); ++i) {
            _agents.push_back({
                0,
                TacticalBehaviour::Normal,
                core::Coordinates{0.0, 0.0},
                &vehicles[i]
            });
        }
    }

    void TrafficSimulationSystem::update(double realTimeDelta) {
        _timeModule.update(realTimeDelta);
        
        _strategicModule.update();
        _tacticalModule.update();
    }

} // tjs
