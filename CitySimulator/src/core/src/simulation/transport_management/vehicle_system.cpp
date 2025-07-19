#include <core/stdafx.h>

#include <core/simulation/transport_management/vehicle_system.h>

#include <core/simulation/simulation_system.h>
#include <core/random_generator.h>

//TODO[simulation]: Probably must move from here while moving further
#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/data_layer/way_info.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/lane_vehicle_utils.h>

namespace tjs::core::simulation {

	VehicleSystem::VehicleSystem(TrafficSimulationSystem& system)
		: _system(system) {
	}

	void VehicleSystem::initialize() {
	}

	void VehicleSystem::release() {
	}

	void VehicleSystem::commit() {
	}

	void VehicleSystem::create_vehicles() {
		auto& settings = _system.settings();
		auto& vehicles = _vehicles;
		vehicles.clear();

		vehicles.reserve(settings.vehiclesCount);

		// Get all nodes from the road network
		auto& segment = _system.worldData().segments()[0];

		std::vector<core::Edge*> all_edges;
		all_edges.reserve(segment->road_network->edges.size());

		for (auto& edge : segment->road_network->edges) {
			all_edges.push_back(&edge);
		}

		// TODO: RandomGenerator<Context>
		if (!settings.randomSeed) {
			RandomGenerator::set_seed(settings.seedValue);
		}

		// Generate vehicles
		for (size_t i = 0; i < settings.vehiclesCount; ++i) {
			// Randomly select a node for the vehicle's coordinates
			auto edge_it = std::next(all_edges.begin(), RandomGenerator::get().next_int(0, all_edges.size() - 1));
			const Coordinates& coordinates = (*edge_it)->start_node->coordinates;

			// Create a vehicle with random attributes and the selected node's coordinates
			Vehicle vehicle;
			vehicle.uid = RandomGenerator::get().next_int(1, 10000000);
			vehicle.type = RandomGenerator::get().next_enum<VehicleType>();
			vehicle.currentSpeed = 0;
			vehicle.maxSpeed = RandomGenerator::get().next_float(40, 100.0f);
			vehicle.coordinates = coordinates;
			vehicle.currentSegmentIndex = 0;
			//vehicle.currentWay = find_way(*nodeIt);
			vehicle.current_lane = &(*edge_it)->lanes[0];
			vehicle.s_on_lane = 0.0;
			vehicle.lateral_offset = 0.0;
			vehicle.state = VehicleState::Stopped;
			vehicle.error = MovementError::None;

			vehicles.push_back(vehicle);
			insert_vehicle_sorted(*vehicle.current_lane, &vehicles.back());
		}
	}

} // namespace tjs::core::simulation
