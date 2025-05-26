#include <core/stdafx.h>

#include <core/simulation/movement/vehicle_movement_module.h>
#include <core/simulation/simulation_system.h>
#include <core/math_constants.h>

namespace tjs::simulation {
    VehicleMovementModule::VehicleMovementModule(TrafficSimulationSystem& system)
        : _system(system) {

    }

    void VehicleMovementModule::initialize() {
    }

    void VehicleMovementModule::release() {
    }

    void VehicleMovementModule::update() {
        auto& agents = _system.agents();


        for (size_t i = 0; i < agents.size(); ++i) {
            update_movement(agents[i]);
        }
    }


    static double to_radians(double degrees) {
        return degrees * core::MathConstants::M_PI / 180.0;
    }

    static double to_degrees(double radians) {
        return radians * 180.0 / core::MathConstants::M_PI;
    }

    double VehicleMovementModule::haversine_distance(const core::Coordinates& a, const core::Coordinates& b) {
        const double lat1 = to_radians(a.latitude);
        const double lon1 = to_radians(a.longitude);
        const double lat2 = to_radians(b.latitude);
        const double lon2 = to_radians(b.longitude);

        const double dlat = lat2 - lat1;
        const double dlon = lon2 - lon1;

        const double a_harv = pow(sin(dlat/2), 2) + 
                            cos(lat1) * cos(lat2) * pow(sin(dlon/2), 2);
        return core::MathConstants::EARTH_RADIUS * 2 * atan2(sqrt(a_harv), sqrt(1 - a_harv));
    }

    static core::Coordinates move_towards(const core::Coordinates& start, const core::Coordinates& end, double distance) {
        if (distance <= 0) return start;

        const double total_dist = VehicleMovementModule::haversine_distance(start, end);
        if (total_dist <= 1e-3) return end;

        const double fraction = distance / total_dist;
        
        const double lat = start.latitude + fraction * (end.latitude - start.latitude);
        const double lon = start.longitude + fraction * (end.longitude - start.longitude);
        
        return {lat, lon};
    }

    void VehicleMovementModule::update_movement(AgentData& agent) {
        auto& worldData = _system.worldData();
        
        agent.vehicle->currentSpeed = 60.0f;
        double delta_time = _system.timeModule().state().timeDelta;
        // Convert km/h to m/s and calculate distance covered this frame
        const double speed_mps = agent.vehicle->currentSpeed * 1000.0 / 3600.0;
        const double max_move = speed_mps * delta_time;

        // Get current and target positions
        const core::Coordinates current = agent.vehicle->coordinates;
        const core::Coordinates target = agent.currentGoal;

        // Calculate actual movement
        const double distance_to_target = haversine_distance(current, target);
        
        if (distance_to_target <= max_move) {
            // Reached destination
            agent.vehicle->coordinates = target;
        } else {
            // Calculate new position
            agent.vehicle->coordinates = move_towards(current, target, max_move);
        }

        // Ensure speed doesn't exceed maximum
        agent.vehicle->currentSpeed = std::min(agent.vehicle->currentSpeed, 
                                            agent.vehicle->maxSpeed);

    }

}