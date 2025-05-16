#include <core/stdafx.h>

#include <core/simulation/tactical/tactical_planning_module.h>

#include <core/simulation/simulation_system.h>

namespace tjs::simulation
{
    
    TacticalPlanningModule::TacticalPlanningModule(TrafficSimulationSystem& system)
        : _system(system) {

    }

    void TacticalPlanningModule::update() {
        auto& agents = _system.agents();    
        for (size_t i = 0; i < agents.size(); ++i) {
            updateAgentTactics(agents[i]);
        }
    }

    void TacticalPlanningModule::updateAgentTactics(tjs::simulation::AgentData& agent) {
        
    }

} // namespace tjs::simulation

