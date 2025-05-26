#pragma once

namespace tjs::core
{
    struct Coordinates;
} // namespace tjs::core


namespace tjs::simulation {

    class TrafficSimulationSystem;
    struct AgentData;

    class VehicleMovementModule {
    public:
        VehicleMovementModule(TrafficSimulationSystem& system);

        void initialize();
        void release();
        void update();


        static double haversine_distance(const core::Coordinates& a, const core::Coordinates& b);

    private:
        void update_movement(AgentData& agent);

    private:
        TrafficSimulationSystem& _system;
    };
}
