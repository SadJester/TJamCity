#pragma once


namespace tjs::simulation {

    class TrafficSimulationSystem;
    struct AgentData;
    
    class VehicleMovementModule {
    public:
        VehicleMovementModule(TrafficSimulationSystem& system);

        void initialize();
        void release();
        void update();

    private:
        void update_movement(AgentData& agent);

    private:
        TrafficSimulationSystem& _system;
    };
}
