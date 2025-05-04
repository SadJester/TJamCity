#include "stdafx.h"

#include "dataLayer/WorldCreator.h"
#include "dataLayer/WorldData.h"


namespace tjs::core {

    void PopulateVehicles(WorldEntries<Vehicle>& vehicles) {
        for (int i = 0; i < 100; ++i) {
            vehicles.push_back(Vehicle {
                i,
                VehicleType::SimpleCar,
                0.f,
                150.f,
                {0.f, 0.f}
            });
        }
    }

    WorldData WorldCreator::createWorld() {
        WorldData world;
        
        PopulateVehicles(world.vehicles());

        return world;
    }
}
