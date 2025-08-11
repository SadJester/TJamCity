#include <core/stdafx.h>

#include <core/simulation/transport_management/vehicle_system.h>

#include <core/simulation/simulation_system.h>
#include <core/random_generator.h>
#include <core/events/vehicle_population_events.h>

#include <core/simulation/transport_management/transport_generator.h>

#include <core/simulation/time_module.h>

//TODO[simulation]: Probably must move from here while moving further
#include <core/data_layer/data_types.h>
#include <core/data_layer/world_data.h>
#include <core/data_layer/way_info.h>
#include <core/data_layer/lane.h>
#include <core/data_layer/lane_vehicle_utils.h>

namespace tjs::core::simulation {

	void create_vehicle_impl(Vehicles& vehicles, VehicleBuffers& buffers, Lane& lane, std::vector<LaneRuntime>& lane_rt, const VehicleSystem::VehicleConfigs& configs, VehicleType type) {
		Vehicle vehicle {};
		vehicle.uid = RandomGenerator::get().next_int(1, 10000000);
		vehicle.type = type;
		auto it_config = configs.find(vehicle.type);
		if (it_config == configs.end()) {
			it_config = configs.begin();
		}

		vehicle.length = it_config->second.length;
		vehicle.width = it_config->second.width;
		vehicle.currentSpeed = 0;
		vehicle.maxSpeed = RandomGenerator::get().next_float(40, 100.0f);
		vehicle.coordinates = lane.parent->start_node->coordinates;
		vehicle.currentSegmentIndex = 0;
		vehicle.current_lane = &lane;
		vehicle.s_on_lane = 0.0;
		vehicle.lateral_offset = 0.0;
		VehicleStateBitsV::set_info(vehicle.state, VehicleStateBits::ST_STOPPED, VehicleStateBitsDivision::STATE);
		vehicle.previous_state = vehicle.state;
		vehicle.error = VehicleMovementError::ER_NO_ERROR;

		vehicles.push_back(vehicle);
		insert_vehicle_sorted(*vehicle.current_lane, &vehicles.back());

		lane_rt[lane.index_in_buffer].idx.push_back(vehicles.size() - 1);

		buffers.add_vehicle(vehicle);
	}

	bool allowed_on_lane(const Lane& lane) {
		// 2 meters from bumper
		return lane.vehicles.empty() || lane.vehicles.back()->s_on_lane > (2.0f + lane.vehicles.back()->length / 2.0f);
	}

	VehicleSystem::VehicleSystem(TrafficSimulationSystem& system)
		: _system(system) {
	}

	VehicleSystem::~VehicleSystem() {
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

		_creation_state = CreationState::InProgress;
		_creation_ticks = 0;
		_buffers.clear();
		_vehicles.clear();
		_vehicles.reserve(_system.settings().vehiclesCount);
		_buffers.reserve(_system.settings().vehiclesCount);
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

	void VehicleSystem::commit() {
		for (size_t i = 0; i < _vehicles.size(); ++i) {
			Vehicle& v = _vehicles[i];

			const bool has_changes =
				v.current_lane != _buffers.lane[i]
				|| v.s_on_lane != _buffers.s_curr[i]
				|| v.lateral_offset != _buffers.lateral_off[i];

			v.current_lane = _buffers.lane[i];
			v.currentSpeed = _buffers.v_curr[i];
			v.s_on_lane = _buffers.s_curr[i];
			v.lateral_offset = _buffers.lateral_off[i];
			v.previous_state = v.state;
			v.state = _buffers.flags[i];

			if (has_changes && v.current_lane) {
				v.coordinates = lane_position(*v.current_lane, v.s_on_lane);
				v.rotationAngle = v.current_lane->rotation_angle;
			}
		}

		for (LaneRuntime& rt : _lane_runtime) {
			Lane& lane = *rt.static_lane;
			auto& idx = rt.idx;
			lane.vehicles.resize(idx.size());
			for (std::size_t i = 0; i < idx.size(); ++i) {
				lane.vehicles[i] = &_vehicles[idx[i]];
			}
		}
	}

	std::optional<size_t> VehicleSystem::create_vehicle(Lane& lane, VehicleType type) {
		if (!allowed_on_lane(lane)) {
			return {};
		}

		create_vehicle_impl(_vehicles, _buffers, lane, _lane_runtime, _vehicle_configs, type);

		return _vehicles.size() - 1;
	}

	void VehicleSystem::update() {
	}

	void VehicleSystem::remove_vehicle(Vehicle& vehicle) {
	}

} // namespace tjs::core::simulation
