#include <core/stdafx.h>

#include <core/simulation/strategic/strategic_planning_module.h>

#include <core/simulation/simulation_system.h>


namespace tjs::simulation
{
    
    StrategicPlanningModule::StrategicPlanningModule(TrafficSimulationSystem& system)
        : _system(system)
    {}

    void StrategicPlanningModule::update() {
        auto& agents = _system.agents();
        for (size_t i = 0; i < agents.size(); ++i) {
            updateAgentStrategy(agents[i]);
        }
    }

    void StrategicPlanningModule::updateAgentStrategy(AgentData& agent) {

    }

} // namespace tjs::simulation



