#pragma once


namespace tjs::core {
    struct Position {
        float x;
        float y;
    };

    enum class VehicleType : char  {
        SimpleCar,
        SmallTruck,
        BigTruck,
        Ambulance,
        PoliceCar,
        FireTrack
    };

    struct Vehicle {
        int id;
        VehicleType type;
        float currentSpeed;
        float maxSpeed;
        Position position;
    };
    static_assert(std::is_pod<Vehicle>::value, "Data object expect to be POD");

    struct Building {
        int id;
    };
    static_assert(std::is_pod<Building>::value, "Data object expect to be POD");

    struct TrafficLight {
        int id;
    };
    static_assert(std::is_pod<TrafficLight>::value, "Data object expect to be POD");


    
}
