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
		if (_system.worldData().segments().empty()) {
			return;
		}

		_vehicle_configs = {
			{ VehicleType::SimpleCar, { VehicleType::SimpleCar, 4.5f, 1.8f } },
			{ VehicleType::SmallTruck, { VehicleType::SmallTruck, 6.5f, 2.5f } },
			{ VehicleType::BigTruck, { VehicleType::BigTruck, 10.0f, 2.5f } },
			{ VehicleType::Ambulance, { VehicleType::Ambulance, 5.5f, 2.2f } },
			{ VehicleType::PoliceCar, { VehicleType::PoliceCar, 5.0f, 2.0f } },
			{ VehicleType::FireTrack, { VehicleType::FireTrack, 8.0f, 2.5f } }
		};

		auto& segment = _system.worldData().segments()[0];
		auto& network = *segment->road_network;

		_lane_runtime.clear();
		for (auto& edge : network.edges) {
			for (auto& lane : edge.lanes) {
				_lane_runtime.push_back({ &lane,
					lane.length,
					edge.way->maxSpeed / 3.6f,
					{} });
				lane.index_in_buffer = _lane_runtime.size() - 1;
			}
		}
	}

	void VehicleSystem::release() {
	}

	static core::Coordinates lane_position(const Lane& lane, double s) {
		if (lane.centerLine.empty()) {
			return {};
		}
		const auto& start = lane.centerLine.front();
		const auto& end = lane.centerLine.back();
		if (s <= 0.0) {
			return start;
		}
		if (lane.length <= 1e-6) {
			return end;
		}
		double fraction = s / lane.length;
		Coordinates result {};
		result.x = start.x + fraction * (end.x - start.x);
		result.y = start.y + fraction * (end.y - start.y);
		return result;
	}

	static float lane_rotation(const Lane& lane) {
		if (lane.centerLine.empty()) {
			return 0.0f;
		}
		const auto& start = lane.centerLine.front();
		const auto& end = lane.centerLine.back();
		return static_cast<float>(atan2(end.y - start.y, end.x - start.x));
	}

	void VehicleSystem::commit() {
		for (size_t i = 0; i < _vehicles.size(); ++i) {
			Vehicle& v = _vehicles[i];
			v.current_lane = _buffers.lane[i];
			v.currentSpeed = _buffers.v_curr[i];
			v.s_on_lane = _buffers.s_curr[i];
			v.lateral_offset = _buffers.lateral_off[i];
			if (v.current_lane) {
				v.coordinates = lane_position(*v.current_lane, v.s_on_lane);
				v.rotationAngle = lane_rotation(*v.current_lane);
			}
		}

		for (LaneRuntime& rt : _lane_runtime) {
			rt.static_lane->vehicles.clear();
			for (std::size_t idx : rt.idx) {
				rt.static_lane->vehicles.push_back(&_vehicles[idx]);
			}
		}
	}

	void VehicleSystem::create_vehicles() {
		if (_system.worldData().segments().empty()) {
			return;
		}

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

			auto it_config = _vehicle_configs.find(vehicle.type);

			if (it_config == _vehicle_configs.end()) {
				// TODO[simulation]: error handling
				it_config = _vehicle_configs.begin();
			}

			vehicle.length = it_config->second.length;
			vehicle.width = it_config->second.width;

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

			for (LaneRuntime& rt : _lane_runtime) {
				if (rt.static_lane == vehicle.current_lane) {
					rt.idx.push_back(vehicles.size() - 1);
					break;
				}
			}
		}

		const size_t count = vehicles.size();
		_buffers.s_curr.resize(count);
		_buffers.s_next.resize(count);
		_buffers.v_curr.resize(count);
		_buffers.v_next.resize(count);
		_buffers.desired_v.resize(count);
		_buffers.length.resize(count);
		_buffers.lateral_off.resize(count);
		_buffers.lane.resize(count);
		_buffers.lane_target.assign(count, nullptr);
		_buffers.flags.resize(count, 0);
		_buffers.v_max_speed.resize(count, 0);
		_buffers.uids.resize(count, 0);

		for (size_t i = 0; i < count; ++i) {
			const Vehicle& v = vehicles[i];
			_buffers.s_curr[i] = v.s_on_lane;
			_buffers.s_next[i] = v.s_on_lane;
			_buffers.v_curr[i] = v.currentSpeed;
			_buffers.v_next[i] = v.currentSpeed;
			_buffers.desired_v[i] = v.maxSpeed;
			_buffers.length[i] = v.length;
			_buffers.lateral_off[i] = v.lateral_offset;
			_buffers.lane[i] = v.current_lane;
			_buffers.flags[i] = 0;
			_buffers.v_max_speed[i] = v.maxSpeed;
			_buffers.uids[i] = v.uid;
		}
	}

} // namespace tjs::core::simulation
