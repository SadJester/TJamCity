#pragma once
#include "core/dataLayer/DataTypes.h"
#include "core/simulation/agent/agent_data.h"

namespace tjs::simulation {
    class TrafficSimulationSystem;

    class TacticalPlanningModule {
    public:
        TacticalPlanningModule(TrafficSimulationSystem& system);

        void update();

    private:
        void updateAgentTactics(tjs::simulation::AgentData& agent);

    private:
        TrafficSimulationSystem& _system;
    };
}
